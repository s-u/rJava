#ifndef __RJAVA_H__
#define __RJAVA_H__

#define RJAVA_VER 0x000103 /* rJava v0.1-3 */

#include <jni.h>

/* in rJava.c */
extern JNIEnv *env;
extern JavaVM *jvm;

/* in callJNI */
jobject createObject(char *class, char *sig, jvalue *par);
jclass getClass(char *class);

jdoubleArray newDoubleArray(double *cont, int len);
jintArray newIntArray(int *cont, int len);
jbooleanArray newBooleanArrayI(int *cont, int len);
jstring newString(char *cont);

void releaseObject(jobject o);
jobject makeGlobal(jobject o);
void releaseGlobal(jobject o);

void printObject(jobject o);
#endif
