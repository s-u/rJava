#ifndef __RJAVA_H__
#define __RJAVA_H__

#define RJAVA_VER 0x000500 /* rJava v0.5-0 */

/* important changes between versions:
   3.0  - adds compiler
   2.0  - integrates JRI, adds callbacks and class-loader
   1.0
   0.5
   0.4  - includes JRI
   0.3  - uses EXTPTR in jobj slot, adds finalizers
   0.2  - uses S4 classes
   0.1  - first public release */

#include <jni.h>
#include <R.h>
#include <Rinternals.h>

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

/* in callbacks.c */
extern int RJava_has_control;

/* in rJava.c */
extern JNIEnv *eenv; /* should NOT be used since not thread-safe; use getJNIEnv instead */

JNIEnv* getJNIEnv();

/* in init.c */
extern JavaVM *jvm;

extern jclass javaStringClass;
extern jclass javaObjectClass;
extern jclass javaClassClass;

/* in Rglue */
SEXP j2SEXP(JNIEnv *env, jobject o, int releaseLocal);
SEXP new_jobjRef(JNIEnv *env, jobject o, const char *klass);
jvalue  R1par2jvalue(JNIEnv *env, SEXP par, char *sig, jobject *otr);

/* in tools.c */
jstring callToString(JNIEnv *env, jobject o);

/* in callJNI */
void init_rJava(void);

jobject createObject(JNIEnv *env, const char *class, const char *sig, jvalue *par, int silent);
jclass findClass(JNIEnv *env, const char *class);
jclass objectClass(JNIEnv *env, jobject o);

jdoubleArray newDoubleArray(JNIEnv *env, double *cont, int len);
jintArray newIntArray(JNIEnv *env, int *cont, int len);
jbooleanArray newBooleanArrayI(JNIEnv *env, int *cont, int len);
jstring newString(JNIEnv *env, const char *cont);
jcharArray newCharArrayI(JNIEnv *env, int *cont, int len);
jshortArray newShortArrayI(JNIEnv *env, int *cont, int len);
jfloatArray newFloatArrayD(JNIEnv *env, double *cont, int len);
jlongArray newLongArrayD(JNIEnv *env, double *cont, int len);
jintArray newByteArray(JNIEnv *env, void *cont, int len);
jbyteArray newByteArrayI(JNIEnv *env, int *cont, int len);

jobject makeGlobal(JNIEnv *env, jobject o);
void releaseObject(JNIEnv *env, jobject o);
void releaseGlobal(JNIEnv *env, jobject o);

void printObject(JNIEnv *env, jobject o);

int checkExceptionsX(JNIEnv *env, int silent);

int initClassLoader(JNIEnv *env, jobject cl);

/* this is a hook for de-serialization, unused for now */
#define jverify(X)

#endif

