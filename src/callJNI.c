#include <stdarg.h>
#include "rJava.h"

void checkExceptions(void);

void* errJNI(char *err, ...) {
  va_list ap;
  va_start(ap, err);
  vfprintf(stderr, err, ap);
  va_end(ap);
  fputs("\n",stderr);
  checkExceptions();
  return 0;
}

/* initialize internal structures/variables of rJava.
   The JVM initialization was performed before (but may have failed)
*/
void init_rJava(void) {
  jclass c;
  if (!env) return; /* initJVM failed, so we cannot proceed */
  
  /* get global classes. we make the references explicitely global (although unloading of String/Object is more than unlikely) */
  c=(*env)->FindClass(env, "java/lang/String");
  if (!c) { errJNI("Unable to find the basic String class"); return; };
  javaStringClass=(*env)->NewGlobalRef(env, c);
  if (!javaStringClass) { errJNI("Unable to create a global reference to the basic String class"); return; };
  (*env)->DeleteLocalRef(env, c);

  c=(*env)->FindClass(env, "java/lang/Object");
  if (!c) { errJNI("Unable to find the basic Object class"); return; };
  javaObjectClass=(*env)->NewGlobalRef(env, c);
  if (!javaObjectClass) { errJNI("Unable to create a global reference to the basic Object class"); return; };
  (*env)->DeleteLocalRef(env, c);
}

jobject createObject(char *class, char *sig, jvalue *par) {
  /* va_list ap; */
  jmethodID mid;
  jclass cls;
  jobject o;

  cls=(*env)->FindClass(env,class);
  if (!cls) return errJNI("createObject.FindClass %s failed",class);
  mid=(*env)->GetMethodID(env, cls, "<init>", sig);
  if (!mid) {
    (*env)->DeleteLocalRef(env, cls);  
    return errJNI("createObject.GetMethodID(\"%s\",\"%s\") failed",class,sig);
  }
  
  /*  va_start(ap, sig); */
  o=(*env)->NewObjectA(env, cls, mid, par);
  /* va_end(ap); */
  (*env)->DeleteLocalRef(env, cls);  
  
  return o?o:errJNI("NewObject(\"%s\",\"%s\",...) failed",class,sig);
}

void printObject(jobject o) {
  jmethodID mid;
  jclass cls;
  jobject s;
  const char *c;

  cls=(*env)->GetObjectClass(env,o);
  if (!cls) return errJNI("printObject.GetObjectClass failed");
  mid=(*env)->GetMethodID(env, cls, "toString", "()Ljava/lang/String;");
  if (!mid) return errJNI("printObject.GetMethodID for toString() failed");
  s=(*env)->CallObjectMethod(env, o, mid);
  if (!s) return errJNI("printObject o.toString() call failed");
  c=(*env)->GetStringUTFChars(env, (jstring)s, 0);
  puts(c);
  (*env)->ReleaseStringUTFChars(env, (jstring)s, c);
  (*env)->DeleteLocalRef(env, cls);  
  releaseObject(s);
}

jclass getClass(char *class) {
  jclass cls;
  cls=(*env)->FindClass(env,class);
  return cls?cls:errJNI("getClass.FindClass %s failed",class);
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

jintArray newIntArray(int *cont, int len) {
  jintArray da=(*env)->NewIntArray(env,len);
  jint *dae;

  if (!da) return errJNI("newIntArray.new(%d) failed",len);
  dae=(*env)->GetIntArrayElements(env, da, 0);
  if (!dae) {
    (*env)->DeleteLocalRef(env,da);
    return errJNI("newIntArray.GetIntArrayElements failed");
  }
  memcpy(dae,cont,sizeof(jint)*len);
  (*env)->ReleaseIntArrayElements(env, da, dae, 0);
  return da;
}

jbooleanArray newBooleanArrayI(int *cont, int len) {
  jbooleanArray da=(*env)->NewBooleanArray(env,len);
  jboolean *dae;
  int i=0;

  if (!da) return errJNI("newBooleanArrayI.new(%d) failed",len);
  dae=(*env)->GetIntArrayElements(env, da, 0);
  if (!dae) {
    (*env)->DeleteLocalRef(env,da);
    return errJNI("newBooleanArrayI.GetBooleanArrayElements failed");
  }
  /* we cannot just memcpy since JNI uses unsigned char and R uses int */
  while (i<len) {
    dae[i]=(jboolean)cont[i];
    i++;
  }
  (*env)->ReleaseBooleanArrayElements(env, da, dae, 0);
  return da;
}

jstring newString(char *cont) {
  jstring s=(*env)->NewStringUTF(env, cont);
  return s?s:errJNI("newString(\"%s\") failed",cont);
}

void releaseObject(jobject o) {
  (*env)->DeleteLocalRef(env, o);
}

jobject makeGlobal(jobject o) {
  jobject g=(*env)->NewGlobalRef(env,o);
  return g?g:errJNI("makeGlobal: failed to make global reference");
}

void releaseGlobal(jobject o) {
  (*env)->DeleteGlobalRef(env,o);
}

void checkExceptions() {
  jthrowable t=(*env)->ExceptionOccurred(env);
  if (t) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
}
