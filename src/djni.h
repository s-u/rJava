/* Dynamic JNI
   Allows to load JVM dynamically into a process.
   
   (C)2024 Simon Urbanek <simon.urbanek@R-project.org>
   License: LGPL-2.1 or MIT
*/
#ifndef DJNI_H__
#define DJNI_H__

#include "jni.h"

int djni_load(const char *path);
int djni_unload(void);

jint JNI_GetDefaultJavaVMInitArgs(void *args);
jint JNI_CreateJavaVM(JavaVM **pvm, void **penv, void *args);
jint JNI_GetCreatedJavaVMs(JavaVM **vmBuf, jsize bufLen, jsize *nVMs);

#endif
