/* Dynamic JNI loader: no JVM dependency is resolved at namespace load. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifdef _WIN32
#define _WIN32_WINNT 0x0602
/* EnumProcessModules is K32EnumProcessModules in kernel32, without psapi.dll */
#define PSAPI_VERSION 2
#include <windows.h>
#include <psapi.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <libproc.h>
#elif defined(__linux__)
#include <link.h>
#include <sys/sysmacros.h>
#endif
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "djni.h"

/* jni.h replaces the JDK headers, so configure no longer compares JNI types
   with native ones. Check the fixed sizes all supported JDKs use instead. */
typedef char djni_jbyte_size[sizeof(jbyte) == 1 ? 1 : -1];
typedef char djni_jboolean_size[sizeof(jboolean) == 1 ? 1 : -1];
typedef char djni_jchar_size[sizeof(jchar) == 2 ? 1 : -1];
typedef char djni_jshort_size[sizeof(jshort) == 2 ? 1 : -1];
typedef char djni_jint_size[sizeof(jint) == 4 ? 1 : -1];
typedef char djni_jlong_size[sizeof(jlong) == 8 ? 1 : -1];
typedef char djni_jfloat_size[sizeof(jfloat) == 4 ? 1 : -1];
typedef char djni_jdouble_size[sizeof(jdouble) == 8 ? 1 : -1];
typedef char djni_jvalue_size[sizeof(jvalue) == 8 ? 1 : -1];

static JNI_GetDefaultJavaVMInitArgs_fnptr JNI_GetDefaultJavaVMInitArgs_fn;
static JNI_CreateJavaVM_fnptr JNI_CreateJavaVM_fn;
static JNI_GetCreatedJavaVMs_fnptr JNI_GetCreatedJavaVMs_fn;
#ifdef _WIN32
static HMODULE jni_dl;
#else
static void *jni_dl;
#endif
static char last_error[1024];
static char *mapped_name;

static void clear_symbols(void) {
    JNI_GetDefaultJavaVMInitArgs_fn = 0;
    JNI_CreateJavaVM_fn = 0;
    JNI_GetCreatedJavaVMs_fn = 0;
}

#ifdef _WIN32
static wchar_t *to_wide(const char *s) {
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, NULL, 0);
    wchar_t *w = (n > 0) ? malloc(sizeof(wchar_t) * n) : NULL;
    if (n > 0 && !w) SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    if (w && MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, w, n) != n) {
        free(w);
        w = NULL;
    }
    return w;
}
static char *to_utf8(const wchar_t *w) {
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
    char *s = (n > 0) ? malloc(n) : NULL;
    if (s && WideCharToMultiByte(CP_UTF8, 0, w, -1, s, n, NULL, NULL) != n) {
        free(s);
        s = NULL;
    }
    return s;
}
static void save_win_error(const char *what, const char *path, DWORD code) {
    wchar_t *msg = NULL;
    char *text = NULL;
    size_t n;
    if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, 0, (LPWSTR) &msg, 0, NULL) && msg)
        text = to_utf8(msg);
    if (msg) LocalFree(msg);
    n = text ? strlen(text) : 0;
    while (n && (text[n - 1] == '\n' || text[n - 1] == '\r' || text[n - 1] == ' ')) text[--n] = 0;
    snprintf(last_error, sizeof(last_error), "%s %s: %s (Windows error %lu)", what, path,
             n ? text : "unknown error", (unsigned long) code);
    free(text);
}
/* A return value of at least the buffer size is the size required, not the
   length of a complete path, so retry instead of using a truncated path. */
static wchar_t *full_path(const wchar_t *path) {
    DWORD size = GetFullPathNameW(path, 0, NULL, NULL), n;
    int tries;
    for (tries = 0; size && tries < 3; tries++) {
        wchar_t *buf = malloc(sizeof(wchar_t) * size);
        if (!buf) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return NULL; }
        n = GetFullPathNameW(path, size, buf, NULL);
        if (n && n < size) return buf;
        free(buf);
        size = n;
    }
    if (size) SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return NULL;
}
/* GetModuleFileNameW returns the buffer size when it truncates. */
static wchar_t *module_path(HMODULE module) {
    DWORD size = MAX_PATH, n;
    for (;;) {
        wchar_t *buf = malloc(sizeof(wchar_t) * size);
        if (!buf) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return NULL; }
        n = GetModuleFileNameW(module, buf, size);
        if (n && n < size) return buf;
        free(buf);
        if (!n) return NULL;
        if (size >= 65536) { SetLastError(ERROR_INSUFFICIENT_BUFFER); return NULL; }
        size *= 2;
    }
}
static HANDLE open_file(const wchar_t *path) {
    return CreateFileW(path, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
}
/* Compare file identities rather than names: 8.3 aliases, junctions,
   symbolic links and spelling variants can all name the same file.
   Returns 1 for the same file, 0 for different files, -1 if unknown. */
static int same_file(const wchar_t *a, const wchar_t *b) {
    HANDLE ha = open_file(a), hb = open_file(b);
    FILE_ID_INFO ia, ib;
    BY_HANDLE_FILE_INFORMATION fa, fb;
    int res = -1;
    if (ha != INVALID_HANDLE_VALUE && hb != INVALID_HANDLE_VALUE) {
        if (GetFileInformationByHandleEx(ha, FileIdInfo, &ia, sizeof(ia)) &&
            GetFileInformationByHandleEx(hb, FileIdInfo, &ib, sizeof(ib)))
            res = ia.VolumeSerialNumber == ib.VolumeSerialNumber &&
                !memcmp(&ia.FileId, &ib.FileId, sizeof(ia.FileId));
        else if (GetFileInformationByHandle(ha, &fa) && GetFileInformationByHandle(hb, &fb))
            res = fa.dwVolumeSerialNumber == fb.dwVolumeSerialNumber &&
                fa.nFileIndexHigh == fb.nFileIndexHigh && fa.nFileIndexLow == fb.nFileIndexLow;
    } else if (hb != INVALID_HANDLE_VALUE)
        res = 0; /* the requested file does not exist, but a JVM is mapped */
    if (ha != INVALID_HANDLE_VALUE) CloseHandle(ha);
    if (hb != INVALID_HANDLE_VALUE) CloseHandle(hb);
    return res;
}
/* The file mapped for a module, as a path that opens it: the module's
   recorded name may now be another file, if the mapped one was renamed and
   replaced. GetMappedFileNameW gives an NT device path. */
static wchar_t *mapped_path(HMODULE module) {
    static const wchar_t root[] = L"\\\\?\\GLOBALROOT";
    DWORD size = 32768, skip = (DWORD) wcslen(root), n;
    wchar_t *buf = malloc(sizeof(wchar_t) * (skip + size));
    if (!buf) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return NULL; }
    wcscpy(buf, root);
    n = K32GetMappedFileNameW(GetCurrentProcess(), (LPVOID) module, buf + skip, size);
    if (n && n < size - 1) return buf;
    if (n) SetLastError(ERROR_INSUFFICIENT_BUFFER);
    free(buf);
    return NULL;
}
static int in_image(DWORD rva, DWORD count, DWORD width, DWORD size) {
    return rva < size && count <= (size - rva) / width;
}
/* Whether a mapped module defines, rather than forwards, an export. The
   export table is read in memory, so nothing is loaded; GetProcAddress
   would load the target of a forwarded export. */
static int owns_export(HMODULE module, const char *sym) {
    const BYTE *base = (const BYTE *) module;
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *) base;
    const IMAGE_NT_HEADERS *nt;
    const IMAGE_DATA_DIRECTORY *dir;
    const IMAGE_EXPORT_DIRECTORY *exp;
    const DWORD *names, *functions;
    const WORD *ordinals;
    DWORD size, i, len = (DWORD) strlen(sym) + 1;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < (LONG) sizeof(*dos) ||
        dos->e_lfanew > 4096 - (LONG) sizeof(*nt))
        return 0;
    nt = (const IMAGE_NT_HEADERS *) (base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC ||
        nt->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXPORT)
        return 0;
    size = nt->OptionalHeader.SizeOfImage;
    dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!dir->VirtualAddress || dir->VirtualAddress >= size || dir->Size < sizeof(*exp) ||
        dir->Size > size - dir->VirtualAddress)
        return 0;
    exp = (const IMAGE_EXPORT_DIRECTORY *) (base + dir->VirtualAddress);
    if (!in_image(exp->AddressOfNames, exp->NumberOfNames, 4, size) ||
        !in_image(exp->AddressOfNameOrdinals, exp->NumberOfNames, 2, size) ||
        !in_image(exp->AddressOfFunctions, exp->NumberOfFunctions, 4, size))
        return 0;
    names = (const DWORD *) (base + exp->AddressOfNames);
    ordinals = (const WORD *) (base + exp->AddressOfNameOrdinals);
    functions = (const DWORD *) (base + exp->AddressOfFunctions);
    /* Names are usually sorted, but a linear scan does not depend on it. */
    for (i = 0; i < exp->NumberOfNames; i++) {
        DWORD f;
        if (names[i] >= size || len > size - names[i] || memcmp(base + names[i], sym, len)) continue;
        if (ordinals[i] >= exp->NumberOfFunctions) return 0;
        f = functions[ordinals[i]];
        /* An address inside the export directory names a forwarded export. */
        return f && f < size && (f < dir->VirtualAddress || f - dir->VirtualAddress >= dir->Size);
    }
    return 0;
}
static int is_jvm_name(const wchar_t *path) {
    const wchar_t *base = wcsrchr(path, L'\\');
    return !_wcsicmp(base ? base + 1 : path, L"jvm.dll");
}
/* A module is a JVM if it is named jvm.dll, even if malformed, or if it
   defines the JNI invocation exports and a JVM_ export, whatever its name.
   JNI exports alone do not suffice: rJava's wrappers have them too. */
static int is_jvm_module(HMODULE module, const wchar_t *path) {
    return (path && is_jvm_name(path)) ||
        (owns_export(module, "JVM_CurrentTimeMillis") && owns_export(module, "JNI_CreateJavaVM") &&
         owns_export(module, "JNI_GetCreatedJavaVMs"));
}
struct jvm_image { HMODULE handle; wchar_t *path; };
/* Lists the JVM modules mapped into the process, each with a reference
   (and path if known). Returns their number, -1 if modules cannot be listed. */
static int find_jvms(struct jvm_image **out) {
    HMODULE *mods = NULL;
    struct jvm_image *v = NULL;
    DWORD need = 0, size = 0, count, i;
    int n = 0, tries;
    *out = NULL;
    for (tries = 0; tries < 4 && need >= size; tries++) {
        HMODULE *m = realloc(mods, (size = need + 64 * sizeof(HMODULE)));
        if (!m) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); goto fail; }
        mods = m;
        if (!K32EnumProcessModules(GetCurrentProcess(), mods, size, &need)) goto fail;
    }
    if (need >= size) { SetLastError(ERROR_INSUFFICIENT_BUFFER); goto fail; }
    count = need / sizeof(HMODULE);
    if (count && !(v = calloc(count, sizeof(*v)))) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); goto fail; }
    for (i = 0; i < count; i++) {
        HMODULE module = NULL;
        wchar_t *path;
        /* Hold a reference while reading the module: a module unloaded since
           the enumeration is skipped, and cannot be replaced while read. */
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR) mods[i], &module)) continue;
        if (module != mods[i]) { FreeLibrary(module); continue; }
        path = module_path(module);
        if (is_jvm_module(module, path)) {
            v[n].handle = module;
            v[n++].path = path;
        } else {
            free(path);
            FreeLibrary(module);
        }
    }
    free(mods);
    *out = v;
    return n;
fail:
    free(mods);
    free(v);
    return -1;
}
static void free_jvms(struct jvm_image *v, int n) {
    int i;
    for (i = 0; i < n; i++) {
        if (v[i].handle) FreeLibrary(v[i].handle);
        free(v[i].path);
    }
    free(v);
}
/* The number of VMs created by a JVM module, or -1 if unknown. */
static int created_vms(HMODULE module) {
    JNI_GetCreatedJavaVMs_fnptr created;
    JavaVM *vm;
    jsize count = 0;
    if (!owns_export(module, "JNI_GetCreatedJavaVMs")) return -1;
    created = (JNI_GetCreatedJavaVMs_fnptr) GetProcAddress(module, "JNI_GetCreatedJavaVMs");
    return created && created(&vm, 1, &count) == JNI_OK ? (int) count : -1;
}
/* File versions let an up-to-date system runtime take precedence over an
   older JDK copy. Unknown versions are not evidence that the JDK is newer. */
static ULONGLONG dll_version(const wchar_t *path) {
    DWORD ignored, size = GetFileVersionInfoSizeW(path, &ignored);
    VS_FIXEDFILEINFO *info;
    UINT len;
    ULONGLONG version = 0;
    void *data = size ? malloc(size) : NULL;
    if (data && GetFileVersionInfoW(path, 0, size, data) &&
        VerQueryValueW(data, L"\\", (void **) &info, &len) &&
        len >= sizeof(*info) && info->dwSignature == 0xfeef04bd)
        version = ((ULONGLONG) info->dwFileVersionMS << 32) | info->dwFileVersionLS;
    free(data);
    return version;
}
/* Retain loaded runtimes without replacing a host's choice. For each DLL
   the JDK bundles, otherwise select the newer of it and System32, preferring
   System32 on a tie or unknown version. Do this before jvm.dll binds imports.
   Unbundled, unloaded runtimes remain the loader's responsibility. References
   last until process exit, without changing PATH or DLL search directories. */
static void retain_msvc(const wchar_t *bin) {
    static const wchar_t *names[] = {
        L"vcruntime140.dll", L"vcruntime140_1.dll", L"msvcp140.dll",
        L"msvcp140_1.dll", L"msvcp140_2.dll", L"msvcp140_atomic_wait.dll"
    };
    static HMODULE retained[sizeof(names) / sizeof(names[0])];
    wchar_t system[MAX_PATH + 64], *bundled;
    UINT slen = GetSystemDirectoryW(system, MAX_PATH);
    size_t i, blen = wcslen(bin);
    if (!slen || slen >= MAX_PATH || !(bundled = malloc(sizeof(wchar_t) * (blen + 64)))) {
        fprintf(stderr, "rJava: cannot prepare MSVC runtime paths.\n");
        return;
    }
    wcscpy(bundled, bin);
    bundled[blen] = L'\\';
    system[slen] = L'\\';
    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        const wchar_t *choice;
        DWORD attrs, error;
        char *name;
        if (retained[i]) continue;
        /* Own a reference even if the original loader later releases its DLL. */
        if (GetModuleHandleExW(0, names[i], &retained[i])) continue;
        wcscpy(bundled + blen + 1, names[i]);
        wcscpy(system + slen + 1, names[i]);
        attrs = GetFileAttributesW(bundled);
        if (attrs == INVALID_FILE_ATTRIBUTES) {
            error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) continue;
            name = to_utf8(bundled);
            fprintf(stderr, "rJava: cannot inspect bundled MSVC runtime %s (Windows error %lu).\n",
                    name ? name : "(unknown path)", (unsigned long) error);
            free(name);
            continue;
        }
        choice = bundled;
        if (GetFileAttributesW(system) != INVALID_FILE_ATTRIBUTES) {
            ULONGLONG vb = dll_version(bundled), vs = dll_version(system);
            if (!vb || !vs || vs >= vb) choice = system;
        }
        retained[i] = LoadLibraryExW(choice, NULL,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (retained[i]) continue;
        error = GetLastError();
        name = to_utf8(choice);
        fprintf(stderr, "rJava: cannot retain MSVC runtime %s (Windows error %lu); later Java native loads may fail.\n",
                name ? name : "(unknown path)", (unsigned long) error);
        free(name);
    }
    free(bundled);
}
static void load_bundled_msvc(void) {
    wchar_t *path = module_path(jni_dl), *slash;
    if (!path) {
        fprintf(stderr, "rJava: cannot determine the selected JVM's MSVC runtime directory (Windows error %lu).\n", (unsigned long) GetLastError());
        return;
    }
    slash = wcsrchr(path, L'\\');
    if (slash) {
        *slash = 0; /* bin\\server */
        slash = wcsrchr(path, L'\\');
        if (slash) { *slash = 0; retain_msvc(path); }
    }
    free(path);
}
static int load_library(const char *path) {
    wchar_t *wide = NULL, *full = NULL, *bin = NULL, *slash;
    struct jvm_image *jvms = NULL;
    DLL_DIRECTORY_COOKIE cookie = NULL;
    int res = -2, n = 0;
    if (!(wide = to_wide(path))) {
        save_win_error("Cannot convert JVM library path to UTF-16", path, GetLastError());
        goto done;
    }
    if (!(full = full_path(wide))) {
        save_win_error("Cannot resolve JVM library path", path, GetLastError());
        goto done;
    }
    for (slash = full; *slash; slash++) if (*slash == L'/') *slash = L'\\';
    if ((n = find_jvms(&jvms)) < 0) {
        save_win_error("Cannot list the modules of", "this process", GetLastError());
        res = -4;
        goto done;
    }
    if (n) {
        /* Two mapped JVM modules are two runtimes, even if they are one file.
           A path selects the mapped module only if it names the mapped file,
           also once a VM was created: djni_attach() attaches without a path. */
        wchar_t *mapped = NULL;
        char *name = jvms[0].path ? to_utf8(jvms[0].path) : NULL;
        int same = -1;
        if (n == 1 && jvms[0].path && (mapped = mapped_path(jvms[0].handle)))
            same = same_file(full, mapped);
        free(mapped);
        if (same == 1 && !is_jvm_name(jvms[0].path)) {
            /* The JDK's DLLs import jvm.dll by name, and the loader matches
               loaded modules by base name: starting this one would load
               another jvm.dll or fail. */
            snprintf(last_error, sizeof(last_error), "The JVM library %s is already mapped under another name: %s. Use a new R process.",
                     path, name ? name : "unknown path");
            res = -4;
        } else if (same == 1) {
            /* Reference the mapped module itself: loading an alias of its path
               must never map a second copy of the JVM. */
            jni_dl = jvms[0].handle;
            jvms[0].handle = NULL;
            res = 0;
        } else {
            if (n > 1)
                snprintf(last_error, sizeof(last_error), "%d JVM libraries are already mapped, including %s. Use a new R process.",
                         n, name ? name : "unknown path");
            else if (same)
                snprintf(last_error, sizeof(last_error), "Cannot verify that %s is the JVM library already mapped: %s. Use a new R process.",
                         path, name ? name : "unknown path");
            else
                snprintf(last_error, sizeof(last_error), "Another JVM library is already mapped: %s. Use a new R process.",
                         name ? name : "unknown path");
            res = -4;
        }
        free(name);
        goto done;
    }
    /* JDK 8 keeps MSVC runtime DLLs in bin, the parent of bin\server. The
       directory is searched only during this load and removed again after
       success or failure, leaving the process DLL search state unchanged. */
    if ((bin = malloc(sizeof(wchar_t) * (wcslen(full) + 1)))) {
        wcscpy(bin, full);
        if ((slash = wcsrchr(bin, L'\\'))) {
            *slash = 0;
            if ((slash = wcsrchr(bin, L'\\'))) {
                *slash = 0;
                retain_msvc(bin);
                cookie = AddDllDirectory(bin);
            }
        }
    }
    jni_dl = LoadLibraryExW(full, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (jni_dl) res = 0;
    else save_win_error("Cannot load JVM library", path, GetLastError());
    if (cookie) RemoveDllDirectory(cookie);
done:
    if (jvms) free_jvms(jvms, n);
    free(wide); free(full); free(bin);
    return res;
}
/* Select the only mapped JVM module. An unstarted module must have its
   usual name, since the JDK DLLs import jvm.dll by name when starting a VM. */
static int attach_library(void) {
    struct jvm_image *jvms;
    char *name;
    int n = find_jvms(&jvms), res = -4, count = -1;
    if (n < 0) {
        save_win_error("Cannot list the modules of", "this process", GetLastError());
        return -4;
    }
    name = (n && jvms[0].path) ? to_utf8(jvms[0].path) : NULL;
    if (!n)
        snprintf(last_error, sizeof(last_error), "No JVM library is mapped into this process.");
    else if (n > 1)
        snprintf(last_error, sizeof(last_error), "%d JVM libraries are already mapped, including %s. Use a new R process.",
                 n, name ? name : "unknown path");
    else if ((count = created_vms(jvms[0].handle)) < 0)
        snprintf(last_error, sizeof(last_error), "Cannot determine the VM count of the mapped JVM library: %s.",
                 name ? name : "unknown path");
    else if (!count && (!jvms[0].path || !is_jvm_name(jvms[0].path)))
        snprintf(last_error, sizeof(last_error), "The JVM library is already mapped under another name: %s. Use a new R process.",
                 name ? name : "unknown path");
    else {
        jni_dl = jvms[0].handle;
        jvms[0].handle = NULL;
        res = 0;
    }
    free(name);
    free_jvms(jvms, n);
    return res;
}
const char *djni_mapped(int *vms) {
    struct jvm_image *jvms;
    int n;
    *vms = -1;
    free(mapped_name);
    mapped_name = NULL;
    if ((n = find_jvms(&jvms)) < 0) {
        save_win_error("Cannot list the modules of", "this process", GetLastError());
        *vms = -2;
        return NULL;
    }
    if (!n) {
        free(jvms);
        return NULL;
    }
    /* With several JVM modules, the VM count of one is not the answer. */
    if (n == 1) *vms = created_vms(jvms[0].handle);
    mapped_name = jvms[0].path ? to_utf8(jvms[0].path) : NULL;
    if (!mapped_name) mapped_name = strdup("(unknown path)");
    if (!mapped_name) {
        snprintf(last_error, sizeof(last_error), "Cannot report the mapped JVM library: out of memory");
        *vms = -2;
    }
    free_jvms(jvms, n);
    return mapped_name;
}
#else
/* Whether path names the file mapped at addr, by device and inode, which
   hold across symbolic links, hard links and equivalent spellings. The name
   the loader recorded is not used: the file there may have been deleted or
   replaced since, while the mapping keeps the old file. Returns 1 for the
   same file, 0 for another or no file, -1 if the mapped file is unknown, and
   -2 for mismatched devices, where an inode mismatch proves nothing, and
   2 for matching inodes on mismatched devices, requiring a loader check. */
static int same_as_mapped(const void *addr, const char *path) {
    struct stat s;
    int res = -1;
    if (stat(path, &s)) return 0;
#ifdef __APPLE__
    {
        struct proc_regionwithpathinfo r;
        uint64_t a = (uint64_t) (uintptr_t) addr;
        /* The region returned starts after addr if nothing is mapped there. */
        if (addr && proc_pidinfo(getpid(), PROC_PIDREGIONPATHINFO, a, &r, sizeof(r)) == (int) sizeof(r) &&
            a >= r.prp_prinfo.pri_address && a - r.prp_prinfo.pri_address < r.prp_prinfo.pri_size &&
            r.prp_vip.vip_vi.vi_stat.vst_ino)
            res = r.prp_vip.vip_vi.vi_stat.vst_dev == (uint32_t) s.st_dev &&
                r.prp_vip.vip_vi.vi_stat.vst_ino == (uint64_t) s.st_ino;
    }
#elif defined(__linux__)
    {
        FILE *f = addr ? fopen("/proc/self/maps", "r") : NULL;
        char *line = NULL;
        size_t size = 0;
        unsigned long long a = (uintptr_t) addr, start, end, ino;
        unsigned int maj = 0, min = 0;
        int device_mismatch;
        if (f) {
            while (getline(&line, &size, f) > 0)
                if (sscanf(line, "%llx-%llx %*s %*s %x:%x %llu", &start, &end, &maj, &min, &ino) == 5 &&
                    a >= start && a < end) {
                    if (ino) {
                        device_mismatch = major(s.st_dev) != maj || minor(s.st_dev) != min;
                        res = device_mismatch ? ((unsigned long long) s.st_ino == ino ? 2 : -2) :
                            (unsigned long long) s.st_ino == ino;
                    }
                    break;
                }
            free(line);
            fclose(f);
        }
        /* btrfs reports a subvolume device to stat(), but the superblock
           device in maps. Overlayfs may report a layer's inode as well.
           Neither device mismatch establishes that this is another file. */
    }
#endif
    return res;
}
static int is_jvm_name(const char *name) {
    const char *base = strrchr(name, '/');
    base = base ? base + 1 : name;
    return !strcmp(base, "libjvm.dylib") || !strcmp(base, "libjvm.so");
}
#ifdef __linux__
/* An image as dl_iterate_phdr() reports it. Its memory is read only while
   the callback runs: glibc holds its loader lock until dl_iterate_phdr()
   returns, and dlclose() unmaps an image only while holding that lock
   (elf/dl-iteratephdr.c, elf/dl-close.c), so any image, Java or not, may be
   unmapped right after. musl never unmaps an image. */
struct elf {
    uintptr_t base;
    const ElfW(Phdr) *phdr;
    size_t phnum;
    const ElfW(Dyn) *dyn;
};
/* Whether [a, a + len) lies in one readable segment of the image. */
static int in_image(const struct elf *im, uintptr_t a, size_t len) {
    size_t i;
    for (i = 0; i < im->phnum; i++) {
        const ElfW(Phdr) *ph = &im->phdr[i];
        uintptr_t start = im->base + (uintptr_t) ph->p_vaddr, end = start + (uintptr_t) ph->p_memsz;
        if (ph->p_type == PT_LOAD && (ph->p_flags & PF_R) && a >= start && a < end && len <= end - a) return 1;
    }
    return 0;
}
/* glibc relocates the addresses of a dynamic section in place on most
   targets, musl never does: an address is used only if it is in the image. */
static uintptr_t dyn_addr(const struct elf *im, uintptr_t v) {
    if (in_image(im, v, 1)) return v;
    if (in_image(im, im->base + v, 1)) return im->base + v;
    return 0;
}
/* The number of entries in the dynamic symbol table, from its hash table as
   musl counts them (ldso/dynlink.c), or -1 if the table cannot be read. */
static long count_syms(const struct elf *im, const uint32_t *hash, const uint32_t *gnu) {
    const uint32_t *buckets, *chain;
    uint32_t i, max = 0;
    if (!gnu) return in_image(im, (uintptr_t) hash, 2 * sizeof(*hash)) ? (long) hash[1] : -1;
    if (!in_image(im, (uintptr_t) gnu, 4 * sizeof(*gnu))) return -1;
    /* The bloom filter words have the size of an address. */
    buckets = (const uint32_t *) ((const ElfW(Addr) *) (gnu + 4) + gnu[2]);
    if (!in_image(im, (uintptr_t) buckets, (size_t) gnu[0] * sizeof(*buckets))) return -1;
    for (i = 0; i < gnu[0]; i++) if (buckets[i] > max) max = buckets[i];
    if (!max) return 0;
    if (max < gnu[1]) return -1;
    /* The chain of the last bucket ends with an odd hash value. */
    for (chain = buckets + gnu[0] + (max - gnu[1]); ; chain++, max++) {
        if (!in_image(im, (uintptr_t) chain, sizeof(*chain))) return -1;
        if (*chain & 1) return (long) max + 1;
    }
}
/* Whether the image defines sym, read from its dynamic symbol table in
   memory as the dynamic loader reads it. Unlike dlsym(), this needs no
   handle: musl takes one by opening the recorded name again, which may since
   name another file or none. Returns 1 if defined, setting *fn to its
   address if it is a function, 0 if not, -1 if the table cannot be read. */
static int image_symbol(const struct elf *im, const char *sym, uintptr_t *fn) {
    const ElfW(Dyn) *d = im->dyn;
    const ElfW(Sym) *syms;
    const char *strs;
    const uint32_t *hash = NULL, *gnu = NULL;
    uintptr_t symtab = 0, strtab = 0, strsz = 0;
    size_t len = strlen(sym) + 1;
    long i, n;
    *fn = 0;
    if (!d) return 0; /* no dynamic section, so no exports */
    for (;; d++) {
        if (!in_image(im, (uintptr_t) d, sizeof(*d))) return -1;
        if (d->d_tag == DT_NULL) break;
        switch (d->d_tag) {
        case DT_SYMTAB: symtab = dyn_addr(im, d->d_un.d_ptr); if (!symtab) return -1; break;
        case DT_STRTAB: strtab = dyn_addr(im, d->d_un.d_ptr); if (!strtab) return -1; break;
        case DT_STRSZ: strsz = d->d_un.d_val; break;
        case DT_HASH: if (!(hash = (const uint32_t *) dyn_addr(im, d->d_un.d_ptr))) return -1; break;
        case DT_GNU_HASH: if (!(gnu = (const uint32_t *) dyn_addr(im, d->d_un.d_ptr))) return -1; break;
        }
    }
    /* Without both tables and a hash table, the loader finds no symbol. */
    if (!symtab || !strtab || (!hash && !gnu)) return 0;
    if (!in_image(im, strtab, strsz) || (n = count_syms(im, hash, gnu)) < 0 ||
        !in_image(im, symtab, (size_t) n * sizeof(*syms)))
        return -1;
    syms = (const ElfW(Sym) *) symtab;
    strs = (const char *) strtab;
    for (i = 0; i < n; i++) {
        const ElfW(Sym) *s = &syms[i];
        if (s->st_shndx == SHN_UNDEF || s->st_name >= strsz || len > strsz - s->st_name ||
            memcmp(strs + s->st_name, sym, len))
            continue;
        /* The low four bits of st_info are the type, in ELF32 and ELF64. */
        if ((s->st_info & 0xf) == STT_FUNC) *fn = im->base + (uintptr_t) s->st_value;
        return 1;
    }
    return 0;
}
#endif
/* An image mapped into the process: its name as the dynamic loader records
   it, and where it is mapped, to tell its own definitions from those of its
   dependencies (macOS). On Linux, only JVM images are listed, with what
   phdr_image() read of them: their dynamic section, which identifies them
   but is not read again, their first readable segment and their
   JNI_GetCreatedJavaVMs. */
struct image {
    char *name;
#ifdef __APPLE__
    const void *header;
#elif defined(__linux__)
    const ElfW(Dyn) *dyn;
    uintptr_t addr, created;
#endif
};
struct images { struct image *v; size_t n, size; int failed; };
static struct image *add_image(struct images *l, const char *name) {
    struct image *im;
    if (l->n == l->size) {
        size_t size = l->size ? 2 * l->size : 64;
        struct image *v = realloc(l->v, size * sizeof(*v));
        if (!v) { l->failed = 1; return NULL; }
        l->v = v;
        l->size = size;
    }
    im = &l->v[l->n];
    memset(im, 0, sizeof(*im));
    if (!(im->name = strdup(name))) { l->failed = 1; return NULL; }
    l->n++;
    return im;
}
#ifdef __linux__
/* Records the JVM images, read only while dl_iterate_phdr() runs (struct
   elf). An image is identified by its exports, not by a handle, which musl
   may be unable to take; an unreadable table is not taken as no JVM. Only
   reads memory: dlopen() must not be called from this callback. */
static int phdr_image(struct dl_phdr_info *info, size_t size, void *data) {
    struct images *l = (struct images *) data;
    struct elf e = { (uintptr_t) info->dlpi_addr, info->dlpi_phdr, info->dlpi_phnum, NULL };
    struct image *im;
    uintptr_t fn, addr = 0, created = 0;
    size_t i;
    int jvm, owned;
    (void) size;
    if (!info->dlpi_name || !*info->dlpi_name) return 0; /* the executable */
    for (i = 0; i < e.phnum; i++) {
        const ElfW(Phdr) *ph = &e.phdr[i];
        if (ph->p_type == PT_LOAD && (ph->p_flags & PF_R) && !addr) addr = e.base + (uintptr_t) ph->p_vaddr;
        if (ph->p_type == PT_DYNAMIC) e.dyn = (const ElfW(Dyn) *) (e.base + (uintptr_t) ph->p_vaddr);
    }
    jvm = is_jvm_name(info->dlpi_name);
    owned = image_symbol(&e, "JVM_CurrentTimeMillis", &fn);
    if (owned > 0) owned = image_symbol(&e, "JNI_CreateJavaVM", &fn);
    if (owned > 0 || jvm) {
        int found = image_symbol(&e, "JNI_GetCreatedJavaVMs", &fn);
        if (owned > 0) owned = found;
        if (found > 0) created = fn;
    }
    if (owned < 0 && !jvm) {
        snprintf(last_error, sizeof(last_error), "Cannot read the exports of %s, mapped into this process", info->dlpi_name);
        l->failed = 2;
        return 1;
    }
    if (!jvm && owned <= 0) return 0;
    if (!(im = add_image(l, info->dlpi_name))) return 1;
    im->dyn = e.dyn;
    im->addr = addr;
    im->created = created;
    return 0;
}
#endif
static void free_images(struct images *l) {
    size_t i;
    for (i = 0; i < l->n; i++) free(l->v[i].name);
    free(l->v);
}
/* Returns 0, or -1 if the images cannot be listed. Other systems list none. */
static int list_images(struct images *l) {
#ifdef __APPLE__
    uint32_t i, n = _dyld_image_count();
    for (i = 0; i < n; i++) {
        const char *name = _dyld_get_image_name(i);
        const struct mach_header *header = _dyld_get_image_header(i);
        struct image *im;
        if (!name || !header) continue;
        if (!(im = add_image(l, name))) break;
        im->header = header;
    }
#elif defined(__linux__)
    dl_iterate_phdr(phdr_image, l);
#endif
    return l->failed ? -1 : 0;
}
#ifdef __APPLE__
/* Whether the image defines sym itself: dlsym() on its handle also finds
   definitions it inherits from the libraries it depends on. */
static int owns(void *handle, const struct image *im, const char *sym) {
    void *p = dlsym(handle, sym);
    Dl_info info;
    return p && dladdr(p, &info) && info.dli_fbase == im->header;
}
#elif defined(__linux__)
/* A reference to the image by name, if the name opens it without loading
   anything. musl matches a name with a slash by the file now there, which
   may be another mapped library, so the handle must be the image's own. */
static void *reference(const ElfW(Dyn) *dyn, const char *name) {
    struct link_map *map;
    void *handle = dlopen(name, RTLD_LAZY | RTLD_LOCAL | RTLD_NOLOAD);
    if (handle && (dlinfo(handle, RTLD_DI_LINKMAP, &map) || (const void *) map->l_ld != (const void *) dyn)) {
        dlclose(handle);
        handle = NULL;
    }
    dlerror(); /* failed lookups are not errors of the caller */
    return handle;
}
#endif
/* A mapped JVM image: its recorded name, a reference to it if one could be
   taken without loading anything, its JNI_GetCreatedJavaVMs, and an address
   in it. */
struct jvm_image {
    char *name;
    void *handle;
    JNI_GetCreatedJavaVMs_fnptr created;
    const void *addr;
#ifdef __linux__
    const ElfW(Dyn) *dyn;
#endif
};
static void free_jvms(struct jvm_image *v, int n) {
    int i;
    for (i = 0; i < n; i++) {
        if (v[i].handle) dlclose(v[i].handle);
        free(v[i].name);
    }
    free(v);
}
/* Lists the JVM images mapped into the process. An image is a JVM if it has
   a JVM library name, even if malformed, or if it defines the JNI invocation
   exports and a JVM_ export, whatever its name. JNI exports alone do not
   suffice: the macOS libjli defines them to forward to libjvm. Returns the
   number of JVM images, or -1 with last_error set if the images cannot be
   listed, or (Linux) the exports of one cannot be read. On Linux, the JVM
   images were identified by phdr_image(), and their functions are called
   only through a reference that holds them mapped (created_vms()). */
static int find_jvms(struct jvm_image **out) {
    struct images l = { NULL, 0, 0, 0 };
    struct jvm_image *v = NULL;
    size_t i;
    int n = 0;
    *out = NULL;
    if (list_images(&l) || (l.n && !(v = calloc(l.n, sizeof(*v))))) {
        if (l.failed != 2) snprintf(last_error, sizeof(last_error), "Cannot list the libraries mapped into this process");
        free_images(&l);
        free(v);
        return -1;
    }
    for (i = 0; i < l.n; i++) {
        struct image *im = &l.v[i];
        void *handle = NULL;
        int jvm = is_jvm_name(im->name);
#ifdef __APPLE__
        /* RTLD_NOLOAD returns a reference only if the image is already mapped,
           and RTLD_LAZY leaves the relocation of other images unchanged. */
        handle = dlopen(im->name, RTLD_LAZY | RTLD_LOCAL | RTLD_NOLOAD);
        if (handle && !jvm)
            jvm = owns(handle, im, "JVM_CurrentTimeMillis") && owns(handle, im, "JNI_CreateJavaVM") &&
                owns(handle, im, "JNI_GetCreatedJavaVMs");
        if (jvm && handle && owns(handle, im, "JNI_GetCreatedJavaVMs"))
            *(void **) &v[n].created = dlsym(handle, "JNI_GetCreatedJavaVMs");
#elif defined(__linux__)
        /* Only JVM images are listed. The recorded name, or the name the
           JDK's libraries bind to. */
        jvm = 1;
        if (!(handle = reference(im->dyn, im->name))) handle = reference(im->dyn, "libjvm.so");
        v[n].dyn = im->dyn;
        v[n].created = (JNI_GetCreatedJavaVMs_fnptr) im->created;
#endif
        if (jvm) {
            v[n].name = im->name;
            im->name = NULL;
#ifdef __APPLE__
            v[n].addr = im->header;
#elif defined(__linux__)
            v[n].addr = (const void *) im->addr;
#endif
            v[n++].handle = handle;
        } else if (handle)
            dlclose(handle);
    }
    dlerror(); /* failed lookups are not errors of the caller */
    free_images(&l);
    *out = v;
    return n;
}
/* The number of VMs created by a JVM image, or -1 if unknown. Its function
   is called only while a reference holds the image mapped. musl, the Linux
   C library assumed when neither glibc nor bionic is, never unmaps an image
   (its dlclose() only checks the handle), so an address read while listing
   the images stays valid without one. */
static int created_vms(const struct jvm_image *im) {
    JavaVM *vm;
    jsize count = 0;
    JNI_GetCreatedJavaVMs_fnptr created = im->created;
#if !defined(__linux__) || defined(__GLIBC__) || defined(__BIONIC__)
    if (!im->handle) return -1;
#endif
#ifdef __linux__
    /* Resolve from the held image, rather than calling an address saved
       before the reference was acquired. A library may have been unmapped
       and another mapped at the same address in that interval. */
    if (im->handle) {
        Dl_info info;
        void *symbol = dlsym(im->handle, "JNI_GetCreatedJavaVMs");
        created = symbol && dladdr(symbol, &info) && info.dli_fbase == im->addr ?
            (JNI_GetCreatedJavaVMs_fnptr) symbol : NULL;
        dlerror();
    }
#endif
    return created && created(&vm, 1, &count) == JNI_OK ? (int) count : -1;
}
static void use_new_process(void) {
    size_t n = strlen(last_error);
    snprintf(last_error + n, sizeof(last_error) - n, ". Use a new R process.");
}
static int load_library(const char *path) {
    struct jvm_image *jvms;
    int n = find_jvms(&jvms), res = -4;
    if (n < 0) {
        use_new_process();
        return -4;
    }
    if (n > 1) {
        snprintf(last_error, sizeof(last_error), "%d JVM libraries are already mapped, including %s. Use a new R process.",
                 n, jvms[0].name);
    } else if (n == 1) {
        /* A path selects the mapped image only if it names the mapped file,
           also once a VM was created: djni_attach() attaches without a path. */
        int same = same_as_mapped(jvms[0].addr, path);
#ifdef __linux__
        if (same == 2) {
            /* A matching inode alone is insufficient across devices. Ask
               the loader whether this path already names this exact image;
               RTLD_NOLOAD never maps the requested file or its dependencies.
               Retain that reference so no second JVM is loaded. */
            void *handle = reference(jvms[0].dyn, path);
            if (handle) {
                if (jvms[0].handle) dlclose(jvms[0].handle);
                jvms[0].handle = handle;
                same = 1;
            } else same = -2;
        }
        /* The path names the mapped file, so it opens the image if the
           names find_jvms() tried no longer do. */
        if (same == 1 && !jvms[0].handle) jvms[0].handle = reference(jvms[0].dyn, path);
#endif
        if (!same)
            snprintf(last_error, sizeof(last_error), "Another JVM library is already mapped: %s. Use a new R process.", jvms[0].name);
        else if (same == -2)
            snprintf(last_error, sizeof(last_error), "Cannot verify that %s is the JVM library already mapped: %s, because the filesystem mapping identity cannot be verified. Use a new R process.",
                     path, jvms[0].name);
        else if (same < 0)
            snprintf(last_error, sizeof(last_error), "Cannot verify that %s is the JVM library already mapped: %s. Use a new R process.", path, jvms[0].name);
        else if (!jvms[0].handle)
            snprintf(last_error, sizeof(last_error), "Cannot reference the JVM library already mapped: %s. Use a new R process.", jvms[0].name);
        else {
            /* Reference the mapped image itself, so an alias cannot map another copy. */
            jni_dl = jvms[0].handle;
            jvms[0].handle = NULL;
            res = 0;
        }
    } else if (!(jni_dl = dlopen(path, RTLD_LOCAL | RTLD_NOW))) {
        const char *error = dlerror();
        snprintf(last_error, sizeof(last_error), "%s", error ? error : "dynamic loader error");
        res = -2;
    } else
        res = 0;
    free_jvms(jvms, n);
    return res;
}
#ifdef __linux__
static int check_jvm_name(const char *path);
#endif
/* Select the only mapped JVM image using a held reference, without loading
   another JVM. Acquiring the reference may inspect the recorded path, whose
   file may have been deleted or replaced. */
static int attach_library(void) {
    struct jvm_image *jvms;
    int n = find_jvms(&jvms), res = -4, count = -1;
    if (n < 0) {
        use_new_process();
        return -4;
    }
    if (!n)
        snprintf(last_error, sizeof(last_error), "No JVM library is mapped into this process.");
    else if (n > 1)
        snprintf(last_error, sizeof(last_error), "%d JVM libraries are already mapped, including %s. Use a new R process.",
                 n, jvms[0].name);
    else if (!jvms[0].handle)
        snprintf(last_error, sizeof(last_error), "Cannot reference the JVM library mapped into this process: %s. Use a new R process.", jvms[0].name);
    else if ((count = created_vms(&jvms[0])) < 0)
        snprintf(last_error, sizeof(last_error), "Cannot determine the VM count of the mapped JVM library: %s.", jvms[0].name);
    else {
        jni_dl = jvms[0].handle;
        jvms[0].handle = NULL;
        res = 0;
#ifdef __linux__
        /* Starting an unstarted JVM still requires its dependencies to bind
           to this very image, including on musl and with renamed libraries. */
        if (!count && (res = check_jvm_name(jvms[0].name))) djni_unload();
#endif
    }
    free_jvms(jvms, n);
    return res;
}
const char *djni_mapped(int *vms) {
    struct jvm_image *jvms;
    int n;
    *vms = -1;
    free(mapped_name);
    mapped_name = NULL;
    if ((n = find_jvms(&jvms)) < 0) {
        *vms = -2;
        return NULL;
    }
    /* With several JVM images, the VM count of one is not the answer. */
    if (n == 1) *vms = created_vms(&jvms[0]);
    if (n) {
        mapped_name = jvms[0].name;
        jvms[0].name = NULL;
    }
    free_jvms(jvms, n);
    return mapped_name;
}
#endif

static int load_sym(const char *sym, void **ptr) {
#ifdef _WIN32
    *ptr = (void *) GetProcAddress(jni_dl, sym);
    if (!*ptr) { save_win_error("Cannot find JNI entry point", sym, GetLastError()); return -1; }
#else
    const char *error;
    *ptr = dlsym(jni_dl, sym);
    if (!*ptr) {
        error = dlerror();
        snprintf(last_error, sizeof(last_error), "%s", error ? error : "dynamic loader error");
        return -1;
    }
#endif
    return 0;
}

/* Resolves the JNI entry points of jni_dl, releasing it if one is missing. */
static int load_syms(void) {
    if (load_sym("JNI_GetDefaultJavaVMInitArgs", (void **) &JNI_GetDefaultJavaVMInitArgs_fn) ||
        load_sym("JNI_CreateJavaVM", (void **) &JNI_CreateJavaVM_fn) ||
        load_sym("JNI_GetCreatedJavaVMs", (void **) &JNI_GetCreatedJavaVMs_fn)) {
        djni_unload();
        return -3;
    }
    return 0;
}

#ifdef __linux__
/* The JDK's own libraries (libjava, libjimage, ...) depend on libjvm.so by
   name, with only their own directory as run path. glibc matches that name
   to the loaded JVM library by its SONAME. musl matches it only to a library
   it found by searching: LD_LIBRARY_PATH as set when the process started,
   then its system path. A JVM library selected by path is then not found,
   or another file is, and starting a VM would map a second JVM library.
   Resolving the name without loading anything shows what they will get. */
static int check_jvm_name(const char *path) {
    void *named = dlopen("libjvm.so", RTLD_LAZY | RTLD_LOCAL | RTLD_NOLOAD);
    const char *slash = strrchr(path, '/');
    int dir = slash ? (int) (slash - path) : 1;
    if (named) dlclose(named);
    dlerror();
    if (named == jni_dl) return 0;
    snprintf(last_error, sizeof(last_error),
             "The JDK's libraries load libjvm.so by name, which in this process does not resolve to %s. "
             "With musl, put %.*s on LD_LIBRARY_PATH before R starts, e.g. with R_JAVA_LD_LIBRARY_PATH. Use a new R process.",
             path, dir, slash ? path : ".");
    return -4;
}
#endif

/* returns: 0 = success, -1 = already loaded, -2 = load error, -3 = symbols
   not found, -4 = another JVM library is already mapped, or (Linux) the
   JDK's libraries would not bind to the selected one */
int djni_load(const char *path) {
    int res;
    last_error[0] = 0;
    if (jni_dl) return -1;
    if ((res = load_library(path))) return res;
    res = load_syms();
#ifdef __linux__
    /* After load_syms(), so that a library that is no JVM is reported as such. */
    if (!res && (res = check_jvm_name(path))) djni_unload();
#endif
#ifdef _WIN32
    if (!res) load_bundled_msvc();
#endif
    return res;
}
/* returns: 0 = success, -1 = already loaded, -3 = symbols not found, -4 = no
   single verifiable mapped JVM library */
int djni_attach(void) {
    int res;
    last_error[0] = 0;
    if (jni_dl) return -1;
    if ((res = attach_library())) return res;
    res = load_syms();
#ifdef _WIN32
    if (!res) load_bundled_msvc();
#endif
    return res;
}
int djni_unload(void) {
    if (!jni_dl) return -1;
#ifdef _WIN32
    if (!FreeLibrary(jni_dl)) { save_win_error("Cannot unload JVM library", "jvm.dll", GetLastError()); return -2; }
#else
    if (dlclose(jni_dl)) {
        const char *error = dlerror();
        snprintf(last_error, sizeof(last_error), "%s", error ? error : "dynamic loader error");
        return -2;
    }
#endif
    jni_dl = 0;
    clear_symbols();
    return 0;
}
int djni_loaded(void) { return jni_dl ? 1 : 0; }
const char *djni_last_error(void) { return last_error[0] ? last_error : 0; }
jint JNI_GetDefaultJavaVMInitArgs(void *args) {
    return JNI_GetDefaultJavaVMInitArgs_fn ? JNI_GetDefaultJavaVMInitArgs_fn(args) : JNI_NO_DJNI;
}
jint JNI_CreateJavaVM(JavaVM **pvm, void **penv, void *args) {
    return JNI_CreateJavaVM_fn ? JNI_CreateJavaVM_fn(pvm, penv, args) : JNI_NO_DJNI;
}
jint JNI_GetCreatedJavaVMs(JavaVM **vmBuf, jsize bufLen, jsize *nVMs) {
    return JNI_GetCreatedJavaVMs_fn ? JNI_GetCreatedJavaVMs_fn(vmBuf, bufLen, nVMs) : JNI_NO_DJNI;
}


/* define DJNI_MAIN_TEST to test the load + create + print version + destroy + unload cycle
   it will create an executable - just compile djni.c alone for this (not used in rJava) */
#ifdef DJNI_MAIN_TEST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* calls java.lang.System.getProperty("java.version") and returns the result or NULL
   if something went wrong. Any non-NULL returns must be free()d */
static char *java_version(JNIEnv *eenv) {
    char *jver = 0;
    jclass sys = (*eenv)->FindClass(eenv, "java/lang/System");
    if (sys) {
	jmethodID mid = (*eenv)->GetStaticMethodID(eenv, sys, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;");
	if (mid) {
	    jstring jvs = (*eenv)->NewStringUTF(eenv, "java.version");
	    if (jvs) {
		jobject jres = (*eenv)->CallStaticObjectMethod(eenv, sys, mid, jvs);
		(*eenv)->DeleteLocalRef(eenv, jvs);
		if (jres) {
		    const char *str = (*eenv)->GetStringUTFChars(eenv, (jstring) jres, NULL);
		    if (str) {
			jver = strdup(str);
			(*eenv)->ReleaseStringUTFChars(eenv, (jstring) jres, str);
		    }
		    (*eenv)->DeleteLocalRef(eenv, jres);
		}
	    }
	}
	(*eenv)->DeleteLocalRef(eenv, sys);
    }
    return jver;
}

/* create a JVM, call java_version(), destroy the JVM */
static int run_jvm_test(void) {
    JavaVM *jvm;
    JNIEnv *eenv;
    JavaVMOption   vm_options[4];
    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_1_2;
    if(JNI_GetDefaultJavaVMInitArgs(&vm_args) != JNI_OK) {
	fprintf(stderr, "JNI 1.2 or higher is required\n");
	return -1;
    }
    vm_args.ignoreUnrecognized = JNI_TRUE;
    vm_args.options = vm_options;
    vm_args.nOptions = 0;
    jint res = JNI_CreateJavaVM(&jvm,(void **)&eenv, &vm_args);
    if (res != 0) {
	fprintf(stderr, "Cannot create Java virtual machine (JNI_CreateJavaVM returned %d)\n", (int) res);
	return -1;
    }
    if (!eenv) {
	fprintf(stderr, "Cannot obtain JVM environment");
	return -1;
    }
    printf("JVM initialized, JNI version: %x\n", (*eenv)->GetVersion(eenv));
    {
	char *jver = java_version(eenv);
	if (jver) {
	    printf("Java version: %s\n", jver);
	    free(jver);
	}
    }
    /* this shouldn't be even needed, but was last-ditch effort - makes no difference */
    if ((*jvm)->DetachCurrentThread(jvm) != JNI_OK)
	fprintf(stderr, "FWIW: cannot detach thread.\n");
    /* destroy the VM */
    if ((*jvm)->DestroyJavaVM(jvm) != JNI_OK) {
	fprintf(stderr, "ERROR: cannot destroy VM\n");
	return -1;
    }
    printf("JVM destroyed.\n");
    return 0;
}

int test_jvm(const char *path) {
    int e = djni_load(path);
    if (e) {
	printf("Cannot load: %d (%s)\n", e, djni_last_error() ? djni_last_error() : "not a dl error");
	return 1;
    }
    printf("JVM Loaded!\n");
    if (!run_jvm_test()) {
	if (djni_unload())
	    fprintf(stderr, "ERROR: cannot unload JNI (%s)\n", djni_last_error() ? djni_last_error() : "no information");
	else
	    printf("JNI unloaded.\n");
    }
    return 0;
}

/* for debugging on macOS - prints all dynamic loads */
#ifdef __APPLE__
#include <mach-o/dyld.h>

static void debug_add_image(const struct mach_header* mh, intptr_t vmaddr_slide) {
    Dl_info DlInfo;
    dladdr(mh, &DlInfo);
    printf("LOADED: %s\n", DlInfo.dli_fname);
}

static void rm_image(const struct mach_header* mh, intptr_t vmaddr_slide) {
    Dl_info DlInfo;
    dladdr(mh, &DlInfo);
    printf("REMOVE: %s\n", DlInfo.dli_fname);
}
#endif

int main(int ac, char **av) {
#ifdef __APPLE__
    _dyld_register_func_for_add_image(debug_add_image);
    _dyld_register_func_for_remove_image(rm_image);
#endif

    test_jvm("/Library/Java/JavaVirtualMachines/temurin-17.jdk/Contents/Home/lib/server/libjvm.dylib");
#ifdef SAME_VM /* option 1: try the same VM twice (will load but fail to create VM) */
    test_jvm("/Library/Java/JavaVirtualMachines/temurin-17.jdk/Contents/Home/lib/server/libjvm.dylib");
#else          /* option 2: will even fail to load */
    test_jvm("/Library/Java/JavaVirtualMachines/temurin-11.jdk/Contents/Home/lib/server/libjvm.dylib");
#endif
    return 0;
}

#endif
