#ifndef __RJAVA_H__
#define __RJAVA_H__

#define RJAVA_VER 0x000201 /* rJava v0.2-1 */

#include <jni.h>

/* in rJava.c */
extern JNIEnv *eenv; /* should NOT be used since not thread-safe; use getJNIEnv instead */
extern JavaVM *jvm;

extern jclass javaStringClass;
extern jclass javaObjectClass;

JNIEnv* getJNIEnv();

int initJVM(char *user_classpath);

/* in callJNI */
void init_rJava(void);

jobject createObject(JNIEnv *env, char *class, char *sig, jvalue *par);
jclass getClass(JNIEnv *env, char *class);

jdoubleArray newDoubleArray(JNIEnv *env, double *cont, int len);
jintArray newIntArray(JNIEnv *env, int *cont, int len);
jbooleanArray newBooleanArrayI(JNIEnv *env, int *cont, int len);
jstring newString(JNIEnv *env, char *cont);

void releaseObject(JNIEnv *env, jobject o);
jobject makeGlobal(JNIEnv *env, jobject o);
void releaseGlobal(JNIEnv *env, jobject o);

void printObject(JNIEnv *env, jobject o);
#endif
