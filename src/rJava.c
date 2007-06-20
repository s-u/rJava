#include <R.h>
#include "rJava.h"
#include <stdlib.h>
#include <string.h>

/* determine whether eenv chache should be used (has no effect if JNI_CACHE is not set) */
int use_eenv = 1;

/* cached environment. Do NOT use directly! Always use getJNIEnv()! */
JNIEnv *eenv;

#ifdef JNI_CACHE
JNIEnv *getJNIEnvSafe();
JNIEnv *getJNIEnv() {
  return (use_eenv)?eenv:getJNIEnvSafe();
}

JNIEnv *getJNIEnvSafe()
#else
JNIEnv *getJNIEnv()
#endif
  {
    JNIEnv *env;
    jsize l;
    jint res;
    
    if (!jvm) { /* we're hoping that the JVM pointer won't change :P we fetch it just once */
        res= JNI_GetCreatedJavaVMs(&jvm, 1, &l);
        if (res!=0) {
            Rf_error("JNI_GetCreatedJavaVMs failed! (result:%d)",(int)res); return 0;
        }
        if (l<1)
	    Rf_error("JNI_GetCreatedJavaVMs said there's no JVM running! Maybe .jinit() would help.");
    }
    res = (*jvm)->AttachCurrentThread(jvm, (void**) &env, 0);
    if (res!=0) {
      Rf_error("AttachCurrentThread failed! (result:%d)", (int)res); return 0;
    }
    if (env && !eenv) eenv=env;
    
    /* if (eenv!=env)
        fprintf(stderr, "Warning! eenv=%x, but env=%x - different environments encountered!\n", eenv, env); */
    return env;
}

void RuseJNICache(int *flag) {
  if (flag) use_eenv=*flag;
}
