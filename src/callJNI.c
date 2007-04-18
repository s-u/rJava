#include <stdarg.h>
#include <string.h>
#include "rJava.h"
#include <R_ext/Print.h>
#include <R_ext/Error.h>

jclass clClassLoader = (jclass) 0;
jobject oClassLoader = (jobject) 0;
static jmethodID midForName;

#ifdef MEMPROF
FILE *memprof_f = 0;
#endif

/* local to JRI */
static void releaseLocal(JNIEnv *env, jobject o);

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
  if (!env) return; /* initJVMfailed, so we cannot proceed */
  
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

  c = (*env)->FindClass(env, "java/lang/Class");
  if (!c) { errJNI("Unable to find the basic Class class"); return; };
  javaClassClass=(*env)->NewGlobalRef(env, c);
  if (!javaClassClass) { errJNI("Unable to create a global reference to the basic Class class"); return; };
  (*env)->DeleteLocalRef(env, c);
}

int initClassLoader(JNIEnv *env, jobject cl) {
  clClassLoader = (*env)->NewGlobalRef(env, (*env)->GetObjectClass(env, cl));
  /* oClassLoader = (*env)->NewGlobalRef(env, cl); */ oClassLoader = cl;
#ifdef DEBUG_CL
  printf("initClassLoader: cl=%x, clCl=%x, jcl=%x\n", oClassLoader, clClassLoader, javaClassClass);
#endif
  midForName = (*env)->GetStaticMethodID(env, javaClassClass, "forName", "(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;");
  return 0;
}

jclass findClass(JNIEnv *env, char *cName) {
  if (clClassLoader) {
    char cn[128], *c=cn;
    jobject cns;
    jclass cl;

    strcpy(cn, cName);
    while (*c) { if (*c=='/') *c='.'; c++; };
    cns = newString(env, cn);
    /* can we pass 1 or do we have to create a boolean object? */
#ifdef DEBUG_CL
    printf("findClass(\"%s\") [with rJava loader]\n", cn);
#endif
    cl = (jclass) (*env)->CallStaticObjectMethod(env, javaClassClass, midForName, cns, (jboolean) 1, oClassLoader);
    _mp(MEM_PROF_OUT("  %08x LNEW class\n", (int) cl))
    releaseObject(env, cns);
#ifdef DEBUG_CL
    printf(" - got %x\n", (unsigned int) cl);
#endif
    if (cl) return cl;
  }
#ifdef DEBUG_CL
  printf("findClass(\"%s\") (no loader)\n", cName);
#endif
  { 
    jclass cl = (*env)->FindClass(env, cName);
    _mp(MEM_PROF_OUT("  %08x LNEW class\n", (int) cl))
#ifdef DEBUG_CL
    printf(" - got %x\n", (unsigned int) cl); 
#endif
    return cl;
  }
}

jobject createObject(JNIEnv *env, char *class, char *sig, jvalue *par, int silent) {
  /* va_list ap; */
  jmethodID mid;
  jclass cls;
  jobject o;

  cls=findClass(env, class);
  if (!cls) return silent?0:errJNI("createObject.FindClass %s failed",class);
  mid=(*env)->GetMethodID(env, cls, "<init>", sig);
  if (!mid) {
    releaseLocal(env, cls);  
    return silent?0:errJNI("createObject.GetMethodID(\"%s\",\"%s\") failed",class,sig);
  }
  
  /*  va_start(ap, sig); */
  o=(*env)->NewObjectA(env, cls, mid, par);
  _mp(MEM_PROF_OUT("  %08x LNEW object\n", (int) o))
  /* va_end(ap); */
  releaseLocal(env, cls);  
  
  return (o||silent)?o:errJNI("NewObject(\"%s\",\"%s\",...) failed",class,sig);
}

void printObject(JNIEnv *env, jobject o) {
  jmethodID mid;
  jclass cls;
  jobject s;
  const char *c;

  cls=(*env)->GetObjectClass(env,o);
  _mp(MEM_PROF_OUT("  %08x LNEW class from object %08x (JRI-local)\n", (int)cls, (int)o))
  if (!cls) { errJNI("printObject.GetObjectClass failed"); releaseLocal(env, cls); return ; }
  mid=(*env)->GetMethodID(env, cls, "toString", "()Ljava/lang/String;");
  if (!mid) { errJNI("printObject.GetMethodID for toString() failed"); releaseLocal(env, cls); return; }
  s=(*env)->CallObjectMethod(env, o, mid);
  _mp(MEM_PROF_OUT("  %08x LNEW object method toString result (JRI-local)\n", (int)s))
  if (!s) { errJNI("printObject o.toString() call failed"); releaseLocal(env, cls); return; }
  c=(*env)->GetStringUTFChars(env, (jstring)s, 0);
  (*env)->ReleaseStringUTFChars(env, (jstring)s, c);
  releaseLocal(env, cls);  
  releaseLocal(env, s);
}

jdoubleArray newDoubleArray(JNIEnv *env, double *cont, int len) {
  jdoubleArray da=(*env)->NewDoubleArray(env,len);
  jdouble *dae;

  _mp(MEM_PROF_OUT("  %08x LNEW double[%d]\n", (int) da, len))
  if (!da) return errJNI("newDoubleArray.new(%d) failed",len);
  dae=(*env)->GetDoubleArrayElements(env, da, 0);
  if (!dae) {
    releaseLocal(env, da);
    return errJNI("newDoubleArray.GetDoubleArrayElements failed");
  }
  memcpy(dae,cont,sizeof(jdouble)*len);
  (*env)->ReleaseDoubleArrayElements(env, da, dae, 0);
  return da;
}

jintArray newIntArray(JNIEnv *env, int *cont, int len) {
  jintArray da=(*env)->NewIntArray(env,len);
  jint *dae;

  _mp(MEM_PROF_OUT("  %08x LNEW int[%d]\n", (int) da, len))
  if (!da) return errJNI("newIntArray.new(%d) failed",len);
  dae=(*env)->GetIntArrayElements(env, da, 0);
  if (!dae) {
    releaseLocal(env,da);
    return errJNI("newIntArray.GetIntArrayElements failed");
  }
  memcpy(dae,cont,sizeof(jint)*len);
  (*env)->ReleaseIntArrayElements(env, da, dae, 0);
  return da;
}

jbyteArray newByteArray(JNIEnv *env, void *cont, int len) {
  jbyteArray da=(*env)->NewByteArray(env,len);
  jbyte *dae;

  _mp(MEM_PROF_OUT("  %08x LNEW byte[%d]\n", (int) da, len))
  if (!da) return errJNI("newByteArray.new(%d) failed",len);
  dae=(*env)->GetByteArrayElements(env, da, 0);
  if (!dae) {
    releaseLocal(env,da);
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

  _mp(MEM_PROF_OUT("  %08x LNEW byte[%d]\n", (int) da, len))
  if (!da) return errJNI("newByteArray.new(%d) failed",len);
  dae=(*env)->GetByteArrayElements(env, da, 0);
  if (!dae) {
    releaseLocal(env,da);
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

  _mp(MEM_PROF_OUT("  %08x LNEW bool[%d]\n", (int) da, len))
  if (!da) return errJNI("newBooleanArrayI.new(%d) failed",len);
  dae=(*env)->GetBooleanArrayElements(env, da, 0);
  if (!dae) {
    releaseLocal(env,da);
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

  _mp(MEM_PROF_OUT("  %08x LNEW char[%d]\n", (int) da, len))
  if (!da) return errJNI("newCharArrayI.new(%d) failed",len);
  dae=(*env)->GetCharArrayElements(env, da, 0);
  if (!dae) {
    releaseLocal(env,da);
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

  _mp(MEM_PROF_OUT("  %08x LNEW float[%d]\n", (int) da, len))
  if (!da) return errJNI("newFloatArrayD.new(%d) failed",len);
  dae=(*env)->GetFloatArrayElements(env, da, 0);
  if (!dae) {
    releaseLocal(env,da);
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
	
	_mp(MEM_PROF_OUT("  %08x LNEW long[%d]\n", (int) da, len))
	if (!da) return errJNI("newLongArrayD.new(%d) failed",len);
	dae=(*env)->GetLongArrayElements(env, da, 0);
	if (!dae) {
	  releaseLocal(env, da);
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
  _mp(MEM_PROF_OUT("  %08x LNEW string \"%s\"\n", (int) s, cont))
  return s?s:errJNI("newString(\"%s\") failed",cont);
}

void releaseObject(JNIEnv *env, jobject o) {
  /* Rprintf("releaseObject: %lx\n", (long)o);
     printObject(env, o); */
  _mp(MEM_PROF_OUT("  %08x LREL\n", (int)o))
  (*env)->DeleteLocalRef(env, o);
}

jclass objectClass(JNIEnv *env, jobject o) {
  jclass cls=(*env)->GetObjectClass(env,o);
  _mp(MEM_PROF_OUT("  %08x LNEW class from object %08x\n", (int) cls, (int) o))
    return cls;
}  

static void releaseLocal(JNIEnv *env, jobject o) {
  _mp(MEM_PROF_OUT("  %08x LREL (JRI-local)\n", (int)o))
  (*env)->DeleteLocalRef(env, o);
}

jobject makeGlobal(JNIEnv *env, jobject o) {
  jobject g=(*env)->NewGlobalRef(env,o);
  _mp(MEM_PROF_OUT("G %08x GNEW %08x\n", (int) g, (int) o))
  return g?g:errJNI("makeGlobal: failed to make global reference");
}

void releaseGlobal(JNIEnv *env, jobject o) {
  /* Rprintf("releaseGlobal: %lx\n", (long)o);
     printObject(env, o); */
  _mp(MEM_PROF_OUT("G %08x GREL\n", (int) o))
  (*env)->DeleteGlobalRef(env,o);
}

static jobject nullEx = 0;

int checkExceptionsX(JNIEnv *env, int silent) {
  jthrowable t=(*env)->ExceptionOccurred(env);
  
  if (t == nullEx) t = 0; else {
    if ((*env)->IsSameObject(env, t, 0)) {
      nullEx = t; t = 0;
    } else {
      _mp(MEM_PROF_OUT("  %08x LNEW exception\n", (int) t))
    }
  }

  if (t) {
    if (!silent)
      (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
    releaseLocal(env, t);
    return 1;
  }
  return 0;
}
