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

/* in callbacks.c */
extern int RJava_has_control;

/* in rJava.c */
extern JNIEnv *eenv; /* should NOT be used since not thread-safe; use getJNIEnv instead */
extern JavaVM *jvm;

extern jclass javaStringClass;
extern jclass javaObjectClass;
extern jclass javaClassClass;

JNIEnv* getJNIEnv();

int initJVM(const char *user_classpath, int opts, char **optv);

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
