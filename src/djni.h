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

int djni_load(const char *path);
int djni_unload(void);
const char* djni_last_error(void);
int djni_loaded(void);

jint JNI_GetDefaultJavaVMInitArgs(void *args);
jint JNI_CreateJavaVM(JavaVM **pvm, void **penv, void *args);
jint JNI_GetCreatedJavaVMs(JavaVM **vmBuf, jsize bufLen, jsize *nVMs);

#endif
