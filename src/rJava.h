#ifndef __RJAVA_H__
#define __RJAVA_H__

#define RJAVA_VER 0x000601 /* rJava v0.6-1 */

/* important changes between versions:
   3.0  - adds compiler
   2.0
   1.0
   0.6  - adds serialization, (auto-)deserialization and cache
   0.5  - integrates JRI, adds callbacks and class-loader
   0.4  - includes JRI
   0.3  - uses EXTPTR in jobj slot, adds finalizers
   0.2  - uses S4 classes
   0.1  - first public release */

#include <jni.h>
#include <R.h>
#include <Rinternals.h>
#include <Rversion.h>

/* flags used in function declarations:
   HIDE - hidden (used internally in rJava only)

   REP  - R Entry Point - .C convention
   REPE - R Entry Point - .External convention
   REPC - R Entry Point - .Call convention

   - inline and/or hide functions that are not externally visible
   - automatically generate symbol registration table
 */
#ifdef ONEFILE
#ifdef HAVE_STATIC_INLINE
#define HIDE static inline
#else
#define HIDE static
#endif
#else
#define HIDE
#endif
#define REP
#define REPE
#define REPC

#if defined WIN32 && ! defined Win32
#define Win32
#endif
#if defined Win32 && ! defined WIN32
#define WIN32
#endif

#ifndef Win32
#include "config.h"
#endif

#ifdef MEMPROF
#include <stdio.h>
#include <time.h>
extern FILE* memprof_f;
#define _mp(X) X
#define MEM_PROF_OUT(X ...) { if (memprof_f) { long t = time(0); fprintf(memprof_f, "<%08x> %x:%02d ", (int) env, t/60, t%60); fprintf(memprof_f, X); }; }
#else
#define _mp(X) 
#endif

/* debugging output (enable with -DRJ_DEBUG) */
#ifdef RJ_DEBUG
void rjprintf(char *fmt, ...); /* in Rglue.c */
/* we can't assume ISO C99 (variadic macros), so we have to use one more level of wrappers */
#define _dbg(X) X
#else
#define _dbg(X)
#endif

/* profiling */
#ifdef RJ_PROFILE
#define profStart() profilerTime=time_ms()
#define _prof(X) X
long time_ms(); /* those are acutally in Rglue.c */
void profReport(char *fmt, ...);
#else
#define profStart()
#define _prof(X)
#endif

#ifdef ENABLE_JRICB
#define BEGIN_RJAVA_CALL { int save_in_RJava = RJava_has_control; RJava_has_control=1; {
#define END_RJAVA_CALL }; RJava_has_control = save_in_RJava; }
#else
#define BEGIN_RJAVA_CALL {
#define END_RJAVA_CALL };
#endif

/* define mkCharUTF8 in a compatible fashion */
#if R_VERSION < R_Version(2,7,0)
#define mkCharUTF8(X) mkChar(X)
#define CHAR_UTF8(X) CHAR(X)
#else
#define mkCharUTF8(X) mkCharCE(X, CE_UTF8)
#define CHAR_UTF8(X) rj_char_utf8(X)
extern const char *rj_char_utf8(SEXP);
#endif

/* in callbacks.c */
extern int RJava_has_control;

/* in rJava.c */
extern JNIEnv *eenv; /* should NOT be used since not thread-safe; use getJNIEnv instead */

HIDE JNIEnv* getJNIEnv();

/* in init.c */
extern JavaVM *jvm;

extern jclass javaStringClass;
extern jclass javaObjectClass;
extern jclass javaClassClass;
extern jclass javaFieldClass;

extern jmethodID mid_forName;
extern jmethodID mid_getName;
extern jmethodID mid_getType;
extern jmethodID mid_getField;

/* in loader.c */
extern jclass   clClassLoader;
extern jobject  oClassLoader;

/* in Rglue */
HIDE SEXP j2SEXP(JNIEnv *env, jobject o, int releaseLocal);
HIDE SEXP new_jobjRef(JNIEnv *env, jobject o, const char *klass);
HIDE jvalue  R1par2jvalue(JNIEnv *env, SEXP par, char *sig, jobject *otr);

/* in tools.c */
HIDE jstring callToString(JNIEnv *env, jobject o);

/* in callJNI */
HIDE jobject createObject(JNIEnv *env, const char *class, const char *sig, jvalue *par, int silent);
HIDE jclass findClass(JNIEnv *env, const char *class);
HIDE jclass objectClass(JNIEnv *env, jobject o);

HIDE jdoubleArray newDoubleArray(JNIEnv *env, double *cont, int len);
HIDE jintArray newIntArray(JNIEnv *env, int *cont, int len);
HIDE jbooleanArray newBooleanArrayI(JNIEnv *env, int *cont, int len);
HIDE jstring newString(JNIEnv *env, const char *cont);
HIDE jcharArray newCharArrayI(JNIEnv *env, int *cont, int len);
HIDE jshortArray newShortArrayI(JNIEnv *env, int *cont, int len);
HIDE jfloatArray newFloatArrayD(JNIEnv *env, double *cont, int len);
HIDE jlongArray newLongArrayD(JNIEnv *env, double *cont, int len);
HIDE jintArray newByteArray(JNIEnv *env, void *cont, int len);
HIDE jbyteArray newByteArrayI(JNIEnv *env, int *cont, int len);

HIDE jobject makeGlobal(JNIEnv *env, jobject o);
HIDE void releaseObject(JNIEnv *env, jobject o);
HIDE void releaseGlobal(JNIEnv *env, jobject o);

HIDE void printObject(JNIEnv *env, jobject o);

HIDE int checkExceptionsX(JNIEnv *env, int silent);

HIDE int initClassLoader(JNIEnv *env, jobject cl);

HIDE void deserializeSEXP(SEXP o);

/* this is a hook for de-serialization */
#define jverify(X) if (EXTPTR_PROT(X) != R_NilValue) deserializeSEXP(X)

#endif

