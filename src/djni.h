/* Dynamic JNI
   Allows to load JVM dynamically into a process.

   (C)2024 Simon Urbanek <simon.urbanek@R-project.org>
   License: LGPL-2.1 or MIT
*/
#ifndef DJNI_H__
#define DJNI_H__

#include "jni.h"

/* returned if no run-time is loaded */
#define JNI_NO_DJNI -99

/* Selects the JVM library at path. If one is already mapped, path must name
   the mapped file, whether or not it has created a VM. */
int djni_load(const char *path);
/* Selects the only JVM library mapped into the process, whether or not it
   has created a VM, by its held image: no additional JVM is loaded.
   A loader lookup may inspect its recorded path to acquire the reference.
   On Windows, selection and attachment retain MSVC runtimes from the JDK
   or System32, preserving already loaded copies, for later Java native loads. */
int djni_attach(void);
int djni_unload(void);
const char* djni_last_error(void);
int djni_loaded(void);
/* path of a JVM library already mapped into the process (or NULL), without
   loading one; *vms is the number of created VMs, -1 if unknown (also if
   several JVM libraries are mapped). NULL with *vms == -2 means the mapped
   libraries could not be listed, see djni_last_error(). */
const char* djni_mapped(int *vms);

/* The wrappers below have the names of the JVM exports. Hide them, so that
   calls from rJava bind to the wrappers even if a JVM library was loaded
   with global symbols, as by the java launcher of an application
   embedding R; ELF symbol interposition would otherwise bypass djni. */
#if defined(__GNUC__) && !defined(_WIN32)
#define DJNI_LOCAL __attribute__((visibility("hidden")))
#else
#define DJNI_LOCAL
#endif

DJNI_LOCAL jint JNI_GetDefaultJavaVMInitArgs(void *args);
DJNI_LOCAL jint JNI_CreateJavaVM(JavaVM **pvm, void **penv, void *args);
DJNI_LOCAL jint JNI_GetCreatedJavaVMs(JavaVM **vmBuf, jsize bufLen, jsize *nVMs);

#endif
