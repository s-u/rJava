#include <stdarg.h>
#include "rJava.h"

void* errJNI(char *err, ...) {
  va_list ap;
  va_start(ap, err);
  vfprintf(stderr, err, ap);
  va_end(ap);
  return 0;
}

jobject createObject(char *class, char *sig, ...) {
  va_list ap;
  jmethodID mid;
  jclass cls;
  jobject o;

  cls=(*env)->FindClass(env,class);
  if (!cls) return errJNI("createObject.FindClass %s failed",class);
  mid=(*env)->GetMethodID(env, cls, "<init>", sig);
  if (!mid) return errJNI("createObject.GetMethodID(\"%s\",\"%s\") failed",class,sig);
  
  va_start(ap, sig);
  o=(*env)->NewObjectV(env, cls, mid, ap);
  va_end(ap);
  return o?o:errJNI("NewObject(\"%s\",\"%s\",...) failed",class,sig);
}

jdoubleArray newDoubleArray(double *cont, int len) {
  jdoubleArray da=(*env)->NewDoubleArray(env,len);
  jdouble *dae;

  if (!da) return errJNI("newDoubleArray.new(%d) failed",len);
  dae=(*env)->GetDoubleArrayElements(env, da, 0);
  if (!dae) {
    (*env)->DeleteLocalRef(env,da);
    return errJNI("newDoubleArray.GetDoubleArrayElements failed");
  }
  memcpy(dae,cont,sizeof(jdouble)*len);
  (*env)->ReleaseDoubleArrayElements(env, da, dae, 0);
  return da;
}

jstring newString(char *cont) {
  jstring s=(*env)->NewStringUTF(env, cont);
  return s?s:errJNI("newString(\"%s\") failed",cont);
}

void releaseObject(jobject o) {
  (*env)->DeleteLocalRef(env, o);
}

