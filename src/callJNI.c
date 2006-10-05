#include <stdarg.h>
#include <string.h>
#include "rJava.h"
#include <R_ext/Print.h>

void checkExceptions() {
  JNIEnv *env=getJNIEnv();
  if (env) checkExceptionsX(env, 0);
}

void RJavaCheckExceptions(int *silent, int *result) {
  JNIEnv *env=getJNIEnv();
  if (env)
    *result=checkExceptionsX(env, *silent);
  else
    *result=0;
}

void* errJNI(char *err, ...) {
  char msg[512];
  va_list ap;
  va_start(ap, err);
  msg[511]=0;
  vsnprintf(msg, 511, err, ap);
  Rf_warning(msg);
  va_end(ap);
  checkExceptions();
  return 0;
}

/* initialize internal structures/variables of rJava.
   The JVM initialization was performed before (but may have failed)
*/
void init_rJava(void) {
  jclass c;
  JNIEnv *env=getJNIEnv();
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

jobject createObject(JNIEnv *env, char *class, char *sig, jvalue *par, int silent) {
  /* va_list ap; */
  jmethodID mid;
  jclass cls;
  jobject o;

  cls=(*env)->FindClass(env,class);
  if (!cls) return silent?0:errJNI("createObject.FindClass %s failed",class);
  mid=(*env)->GetMethodID(env, cls, "<init>", sig);
  if (!mid) {
    (*env)->DeleteLocalRef(env, cls);  
    return silent?0:errJNI("createObject.GetMethodID(\"%s\",\"%s\") failed",class,sig);
  }
  
  /*  va_start(ap, sig); */
  o=(*env)->NewObjectA(env, cls, mid, par);
  /* va_end(ap); */
  (*env)->DeleteLocalRef(env, cls);  
  
  return (o||silent)?o:errJNI("NewObject(\"%s\",\"%s\",...) failed",class,sig);
}

void printObject(JNIEnv *env, jobject o) {
  jmethodID mid;
  jclass cls;
  jobject s;
  const char *c;

  cls=(*env)->GetObjectClass(env,o);
  if (!cls) { errJNI("printObject.GetObjectClass failed"); return ; }
  mid=(*env)->GetMethodID(env, cls, "toString", "()Ljava/lang/String;");
  if (!mid) { errJNI("printObject.GetMethodID for toString() failed"); return; }
  s=(*env)->CallObjectMethod(env, o, mid);
  if (!s) { errJNI("printObject o.toString() call failed"); return; }
  c=(*env)->GetStringUTFChars(env, (jstring)s, 0);
  Rprintf("%s\n",c);
  (*env)->ReleaseStringUTFChars(env, (jstring)s, c);
  (*env)->DeleteLocalRef(env, cls);  
  (*env)->DeleteLocalRef(env, s);
}

jclass getClass(JNIEnv *env, char *class) {
  jclass cls;
  cls=(*env)->FindClass(env,class);
  return cls?cls:errJNI("getClass.FindClass %s failed",class);
}

jdoubleArray newDoubleArray(JNIEnv *env, double *cont, int len) {
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

jintArray newIntArray(JNIEnv *env, int *cont, int len) {
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

jbyteArray newByteArray(JNIEnv *env, void *cont, int len) {
  jbyteArray da=(*env)->NewByteArray(env,len);
  jbyte *dae;

  if (!da) return errJNI("newByteArray.new(%d) failed",len);
  dae=(*env)->GetByteArrayElements(env, da, 0);
  if (!dae) {
    (*env)->DeleteLocalRef(env,da);
    return errJNI("newByteArray.GetByteArrayElements failed");
  }
  memcpy(dae,cont,len);
  (*env)->ReleaseByteArrayElements(env, da, dae, 0);
  return da;
}

jbyteArray newByteArrayI(JNIEnv *env, int *cont, int len) {
  jbyteArray da=(*env)->NewByteArray(env,len);
  jbyte* dae;
  int i=0;

  if (!da) return errJNI("newByteArray.new(%d) failed",len);
  dae=(*env)->GetByteArrayElements(env, da, 0);
  if (!dae) {
    (*env)->DeleteLocalRef(env,da);
    return errJNI("newByteArray.GetByteArrayElements failed");
  }
  while (i<len) {
    dae[i]=(jbyte)cont[i];
    i++;
  }
  (*env)->ReleaseByteArrayElements(env, da, dae, 0);
  return da;
}

jbooleanArray newBooleanArrayI(JNIEnv *env, int *cont, int len) {
  jbooleanArray da=(*env)->NewBooleanArray(env,len);
  jboolean *dae;
  int i=0;

  if (!da) return errJNI("newBooleanArrayI.new(%d) failed",len);
  dae=(*env)->GetBooleanArrayElements(env, da, 0);
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

jcharArray newCharArrayI(JNIEnv *env, int *cont, int len) {
  jcharArray da=(*env)->NewCharArray(env,len);
  jchar *dae;
  int i=0;

  if (!da) return errJNI("newCharArrayI.new(%d) failed",len);
  dae=(*env)->GetCharArrayElements(env, da, 0);
  if (!dae) {
    (*env)->DeleteLocalRef(env,da);
    return errJNI("newCharArrayI.GetCharArrayElements failed");
  }
  while (i<len) {
    dae[i]=(jchar)cont[i];
    i++;
  }
  (*env)->ReleaseCharArrayElements(env, da, dae, 0);
  return da;
}

jfloatArray newFloatArrayD(JNIEnv *env, double *cont, int len) {
  jfloatArray da=(*env)->NewFloatArray(env,len);
  jfloat *dae;
  int i=0;

  if (!da) return errJNI("newFloatArrayD.new(%d) failed",len);
  dae=(*env)->GetFloatArrayElements(env, da, 0);
  if (!dae) {
    (*env)->DeleteLocalRef(env,da);
    return errJNI("newFloatArrayD.GetFloatArrayElements failed");
  }
  /* we cannot just memcpy since JNI uses float and R uses double */
  while (i<len) {
    dae[i]=(jfloat)cont[i];
    i++;
  }
  (*env)->ReleaseFloatArrayElements(env, da, dae, 0);
  return da;
}

jlongArray newLongArrayD(JNIEnv *env, double *cont, int len) {
	jlongArray da=(*env)->NewLongArray(env,len);
	jlong *dae;
	int i=0;
	
	if (!da) return errJNI("newLongArrayD.new(%d) failed",len);
	dae=(*env)->GetLongArrayElements(env, da, 0);
	if (!dae) {
		(*env)->DeleteLocalRef(env,da);
		return errJNI("newLongArrayD.GetFloatArrayElements failed");
	}
	/* we cannot just memcpy since JNI uses long and R uses double */
	while (i<len) {
		/* we're effectively rounding to prevent representation issues
		   however, we still may introduce some errors this way */
		dae[i]=(jlong)(cont[i]+0.5);
		i++;
	}
	(*env)->ReleaseLongArrayElements(env, da, dae, 0);
	return da;
}

jstring newString(JNIEnv *env, char *cont) {
  jstring s=(*env)->NewStringUTF(env, cont);
  return s?s:errJNI("newString(\"%s\") failed",cont);
}

void releaseObject(JNIEnv *env, jobject o) {
  /* Rprintf("releaseObject: %lx\n", (long)o);
     printObject(env, o); */
  (*env)->DeleteLocalRef(env, o);
}

jobject makeGlobal(JNIEnv *env, jobject o) {
  jobject g=(*env)->NewGlobalRef(env,o);
  return g?g:errJNI("makeGlobal: failed to make global reference");
}

void releaseGlobal(JNIEnv *env, jobject o) {
  /* Rprintf("releaseGlobal: %lx\n", (long)o);
     printObject(env, o); */
  (*env)->DeleteGlobalRef(env,o);
}

int checkExceptionsX(JNIEnv *env, int silent) {
  jthrowable t=(*env)->ExceptionOccurred(env);
  if (t) {
    if (!silent)
      (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
    return 1;
  }
  return 0;
}
