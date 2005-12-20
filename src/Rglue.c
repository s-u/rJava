#include "rJava.h"
#include <R.h>
#include <Rdefines.h>
#include <R_ext/Parse.h>

#include <stdarg.h>

/* debugging output (enable with -DRJ_DEBUG) */
#ifdef RJ_DEBUG
void rjprintf(char *fmt, ...) {
  va_list v;
  va_start(v,fmt);
  vprintf(fmt,v);
  va_end(v);
}
#else
#define rjprintf(...)
#endif

/* profiling code (enable with -DRJ_PROFILE) */
#ifdef RJ_PROFILE
#include <sys/time.h>

long time_ms() {
#ifdef Win32
  return 0; /* in Win32 we have no gettimeofday :( */
#else
  struct timeval tv;
  gettimeofday(&tv,0);
  return (tv.tv_usec/1000)+(tv.tv_sec*1000);
#endif
}

long profilerTime;

#define profStart() profilerTime=time_ms()
void profReport(char *fmt, ...) {
  long npt=time_ms();
  va_list v;
  va_start(v,fmt);
  vprintf(fmt,v);
  va_end(v);
  printf(" %ld ms\n",npt-profilerTime);
  profilerTime=npt;
}
#else
#define profStart()
#define profReport(...)
#endif

jstring callToString(jobject o);

void JRefObjectFinalizer(SEXP ref) {
  if (TYPEOF(ref)==EXTPTRSXP) {
    JNIEnv *env=getJNIEnv();
    jobject o = R_ExternalPtrAddr(ref);

#ifdef RJ_DEBUG
    {
      jstring s=callToString(o);
      const char *c=(*env)->GetStringUTFChars(env, s, 0);
      rjprintf("Finalizer releases Java object [%s] reference %lx\n", c, (long)o);
      (*env)->ReleaseStringUTFChars(env, s, c);
    }
#endif

    if (env && o)
      releaseGlobal(env, o);
  }
}

/* jobject to SEXP encoding - 0.2 and earlier use INTSXP */
SEXP j2SEXP(jobject o) {
  SEXP xp = R_MakeExternalPtr(o, R_NilValue, R_NilValue);
  R_RegisterCFinalizerEx(xp, JRefObjectFinalizer, TRUE);
  return xp;
}

#ifdef THREADS
#include <pthread.h>

#ifdef XXDARWIN
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFRunLoop.h>
#endif

pthread_t initJVMpt;
pthread_mutex_t initMutex = PTHREAD_MUTEX_INITIALIZER;

void *initJVMthread(void *classpath)
{
  int ws;
  int r=initJVM((char*)classpath);
  init_rJava();

  pthread_mutex_unlock(&initMutex);

#ifdef XXDARWIN
  CFRunLoopRun();
#else
  while (1) {
    wait(&ws);
  }
#endif
  return 0;
}

#endif

/** RinitJVM(classpath)
    initializes JVM with the specified class path */
SEXP RinitJVM(SEXP par)
{
  char *c=0;
  SEXP e=CADR(par);
  int r=0;
  
  JavaVM *jvms[32];
  jsize vms=0;

  if (TYPEOF(e)==STRSXP && LENGTH(e)>0)
    c=CHAR(STRING_ELT(e,0));

  r=JNI_GetCreatedJavaVMs(jvms, 32, &vms);
  if (r) {
    fprintf(stderr, "JNI_GetCreatedJavaVMs returned %d\n", r);
  } else {
    if (vms>0) {
      int i=0;
      printf("Total %d JVMs found. Trying to attach the current thread.\n", (int)vms);
      while (i<vms) {
	if (jvms[i]) {
	  if (!(*jvms[i])->AttachCurrentThread(jvms[i], (void**)&eenv, NULL)) {
            printf("Attached to existing JVM #%d.\n", i+1);
	    break;
	  }
	}
	i++;
      }
      if (i==vms) fprintf(stderr, "Failed to attach to any existing JVM!\n");
      else {
        init_rJava();
      }
      PROTECT(e=allocVector(INTSXP,1));
      INTEGER(e)[0]=(i==vms)?-2:1;
      UNPROTECT(1);
      return e;
    }
  }

#ifdef THREADS
  printf("launching thread\n");
  pthread_mutex_lock(&initMutex);
  pthread_create(&initJVMpt, 0, initJVMthread, c);
  printf("waiting for mutex\n");
  pthread_mutex_lock(&initMutex);
  pthread_mutex_unlock(&initMutex);
  /* pthread_join(initJVMpt, 0); */
  printf("attach\n");
  /* since JVM was initialized by another thread, we need to attach ourselves */
  (*jvm)->AttachCurrentThread(jvm, (void**)&env, NULL);
  printf("done.\n");
#else
  profStart();
  r=initJVM(c);
  profReport("initJVM:");
  init_rJava();
  profReport("init_rJava:");
#endif
  PROTECT(e=allocVector(INTSXP,1));
  INTEGER(e)[0]=r;
  UNPROTECT(1);
  return e;
}

/** converts parameters in SEXP list to jpar and sig.
    strcat is used on sig, hence sig must be a valid string already */
SEXP Rpar2jvalue(JNIEnv *env, SEXP par, jvalue *jpar, char *sig, int maxpar, int maxsig) {
  SEXP p=par;
  SEXP e;
  int jvpos=0;
  int i=0;

  while (p && TYPEOF(p)==LISTSXP && (e=CAR(p))) {
    rjprintf("par %d type %d\n",i,TYPEOF(e));
    if (TYPEOF(e)==STRSXP) {
      rjprintf(" string vector of length %d\n",LENGTH(e));
      if (LENGTH(e)==1) {
	strcat(sig,"Ljava/lang/String;");
	jpar[jvpos++].l=newString(env, CHAR(STRING_ELT(e,0)));
      } else {
	int j=0;
	jobjectArray sa=(*env)->NewObjectArray(env, LENGTH(e), javaStringClass, 0);
	if (!sa) error_return("Unable to create string array.");
	while (j<LENGTH(e)) {
	  jobject s=newString(env, CHAR(STRING_ELT(e,j)));
	  rjprintf (" [%d] \"%s\"\n",j,CHAR(STRING_ELT(e,j)));
	  (*env)->SetObjectArrayElement(env,sa,j,s);
	  j++;
	}
	jpar[jvpos++].l=sa;
	strcat(sig,"[Ljava/lang/String;");
      }
    } else if (TYPEOF(e)==INTSXP) {
      rjprintf(" integer vector of length %d\n",LENGTH(e));
      if (LENGTH(e)==1) {
	strcat(sig,"I");
	jpar[jvpos++].i=(jint)(INTEGER(e)[0]);
	rjprintf("  single int orig=%d, jarg=%d [jvpos=%d]\n",
	       (INTEGER(e)[0]),
	       jpar[jvpos-1],
	       jvpos);
      } else {
	strcat(sig,"[I");
	jpar[jvpos++].l=newIntArray(env, INTEGER(e),LENGTH(e));
      }
    } else if (TYPEOF(e)==REALSXP) {
      rjprintf(" real vector of length %d\n",LENGTH(e));
      if (LENGTH(e)==1) {
	strcat(sig,"D");
	jpar[jvpos++].d=(jdouble)(REAL(e)[0]);
      } else {
	strcat(sig,"[D");
	jpar[jvpos++].l=newDoubleArray(env, REAL(e),LENGTH(e));
      }
    } else if (TYPEOF(e)==LGLSXP) {
      rjprintf(" logical vector of length %d\n",LENGTH(e));
      if (LENGTH(e)==1) {
	strcat(sig,"Z");
	jpar[jvpos++].z=(jboolean)(LOGICAL(e)[0]);
      } else {
	strcat(sig,"[Z");
	jpar[jvpos++].l=newBooleanArrayI(env, LOGICAL(e),LENGTH(e));
      }
    } else if (TYPEOF(e)==VECSXP) {
      rjprintf(" general vector of length %d\n", LENGTH(e));
      if (inherits(e,"jobjRef")||inherits(e,"jarrayRef")) {
	jobject o=(jobject)0;
	char *jc=0;
	SEXP n=getAttrib(e, R_NamesSymbol);
	if (TYPEOF(n)!=STRSXP) n=0;
	rjprintf(" which is in fact a Java object reference\n");
	if (LENGTH(e)>1) { /* old objects were lists */
	  error("Old, unsupported S3 Java object encountered.");
	} else { /* new objects are S4 objects */
	  SEXP sref, sclass;
	  sref=GET_SLOT(e, install("jobj"));
	  if (sref && TYPEOF(sref)==EXTPTRSXP)
	    o=(jobject)EXTPTR_PTR(sref);
	  else /* if jobj is anything else, assume NULL ptr */
	    o=(jobject)0;
	  sclass=GET_SLOT(e, install("jclass"));
	  if (sclass && TYPEOF(sclass)==STRSXP && LENGTH(sclass)==1)
	    jc=CHAR(STRING_ELT(sclass,0));
	}
	if (jc) {
	  if (*jc!='[') { /* not an array, we assume it's an object of that class */
	    strcat(sig,"L"); strcat(sig,jc); strcat(sig,";");
	  } else /* array definitions are passed as-is */
	    strcat(sig,jc);
	} else
	  strcat(sig,"Ljava/lang/Object;");
	jpar[jvpos++].l=o;
      } else {
	rjprintf(" (ignoring)\n");
      }
    }
    i++;
    p=CDR(p);
  }
  return R_NilValue;
}

/** jobjRefInt object : string */
SEXP RgetStringValue(SEXP par) {
  SEXP p,e,r;
  jstring s;
  const char *c;
  JNIEnv *env=getJNIEnv();
  
  profStart();
  p=CDR(par); e=CAR(p); p=CDR(p);
  if (e==R_NilValue) return R_NilValue;
  if (TYPEOF(e)==EXTPTRSXP)
    s=(jstring)EXTPTR_PTR(e);
  else
    error_return("RgetStringValue: invalid object parameter");
  if (!s) return R_NilValue;
  c=(*env)->GetStringUTFChars(env, s, 0);
  if (!c)
    error_return("RgetStringValue: can't retrieve string content");
  PROTECT(r=allocVector(STRSXP,1));
  SET_STRING_ELT(r, 0, mkChar(c));
  UNPROTECT(1);
  (*env)->ReleaseStringUTFChars(env, s, c);
  profReport("RgetStringValue:");
  return r;
}

/** calls .toString() of the object and returns the corresponding string java object */
jstring callToString(jobject o) {
  jclass cls;
  jmethodID mid;
  JNIEnv *env=getJNIEnv();

  cls=(*env)->GetObjectClass(env,o);
  if (!cls) error("RtoString: can't determine class of the object");
  mid=(*env)->GetMethodID(env, cls, "toString", "()Ljava/lang/String;");
  if (!mid) error("RtoString: toString not found for the object");
  return (jstring)(*env)->CallObjectMethod(env, o, mid);  
}

/** calls .toString() on the passed object (int/extptr) and returns the string 
    value */
SEXP RtoString(SEXP par) {
  SEXP p,e,r;
  jstring s;
  jobject o;
  const char *c;
  JNIEnv *env=getJNIEnv();

  p=CDR(par); e=CAR(p); p=CDR(p);
  if (e==R_NilValue) return R_NilValue;
  if (TYPEOF(e)==EXTPTRSXP)
    o=(jobject)EXTPTR_PTR(e);
  else
    error_return("RtoString: invalid object parameter");
  if (!o) return R_NilValue;
  s=callToString(o);
  if (!s) error_return("RtoString: toString call failed");
  c=(*env)->GetStringUTFChars(env, s, 0);
  PROTECT(r=allocVector(STRSXP,1));
  SET_STRING_ELT(r, 0, mkChar(c));
  UNPROTECT(1);
  (*env)->ReleaseStringUTFChars(env, s, c);
  return r;
}

/** get contents of the object array in the form of list of ext. pointers */
SEXP RgetObjectArrayCont(SEXP par) {
  SEXP e=CAR(CDR(par));
  SEXP ar;
  jarray o;
  int l,i;
  JNIEnv *env=getJNIEnv();

  profStart();
  if (e==R_NilValue) return R_NilValue;
  if (TYPEOF(e)==EXTPTRSXP)
    o=(jobject)EXTPTR_PTR(e);
  else
    error_return("RgetObjectArrayCont: invalid object parameter");
  rjprintf(" jarray %d\n",o);
  if (!o) return R_NilValue;
  l=(int)(*env)->GetArrayLength(env, o);
  rjprintf("convert object array of length %d\n",l);
  if (l<1) return R_NilValue;
  PROTECT(ar=allocVector(VECSXP,l));
  i=0;
  while (i<l) {
    SET_VECTOR_ELT(ar, i, j2SEXP((*env)->GetObjectArrayElement(env, o, i)));
    i++;
  }
  UNPROTECT(1);
  profReport("RgetObjectArrayCont[%d]:",o);
  return ar;
}

/** get contents of the object array in the form of int* */
SEXP RgetStringArrayCont(SEXP par) {
  SEXP e=CAR(CDR(par));
  SEXP ar;
  jarray o;
  int l,i;
  const char *c;
  JNIEnv *env=getJNIEnv();

  profStart();
  if (e==R_NilValue) return R_NilValue;
  if (TYPEOF(e)==EXTPTRSXP)
    o=(jobject)EXTPTR_PTR(e);
  else
    error_return("RgetStringArrayCont: invalid object parameter");
  rjprintf(" jarray %d\n",o);
  if (!o) return R_NilValue;
  l=(int)(*env)->GetArrayLength(env, o);
  rjprintf("convert string array of length %d\n",l);
  if (l<0) return R_NilValue;
  PROTECT(ar=allocVector(STRSXP,l));
  i=0;
  while (i<l) {
    jobject sobj=(*env)->GetObjectArrayElement(env, o, i);
    c=0;
    if (sobj) {
      /* we could (should?) check the type here ...
      if (!(*env)->IsInstanceOf(env, sobj, javaStringClass)) {
	printf(" not a String\n");
      } else
      */
      c=(*env)->GetStringUTFChars(env, sobj, 0);
    }
    if (!c)
      SET_STRING_ELT(ar, i, R_NaString);
    else {
      SET_STRING_ELT(ar, i, mkChar(c));
      (*env)->ReleaseStringUTFChars(env, sobj, c);
    }
    i++;
  }
  UNPROTECT(1);
  profReport("RgetStringArrayCont[%d]:",o);
  return ar;
}

/** get contents of the integer array object */
SEXP RgetIntArrayCont(SEXP par) {
  SEXP e=CAR(CDR(par));
  SEXP ar;
  jarray o;
  int l;
  jint *ap;
  JNIEnv *env=getJNIEnv();

  profStart();
  if (e==R_NilValue) return e;
  if (TYPEOF(e)==EXTPTRSXP)
    o=(jobject)EXTPTR_PTR(e);
  else
    error_return("RgetIntArrayCont: invalid object parameter");
  rjprintf(" jarray %d\n",o);
  if (!o) return R_NilValue;
  l=(int)(*env)->GetArrayLength(env, o);
  rjprintf("convert int array of length %d\n",l);
  if (l<0) return R_NilValue;
  ap=(jint*)(*env)->GetIntArrayElements(env, o, 0);
  if (!ap)
    error_return("RgetIntArrayCont: can't fetch array contents");
  PROTECT(ar=allocVector(INTSXP,l));
  if (l>0) memcpy(INTEGER(ar),ap,sizeof(jint)*l);
  UNPROTECT(1);
  (*env)->ReleaseIntArrayElements(env, o, ap, 0);
  profReport("RgetIntArrayCont[%d]:",o);
  return ar;
}

/** get contents of the double array object  */
SEXP RgetDoubleArrayCont(SEXP par) {
  SEXP e=CAR(CDR(par));
  SEXP ar;
  jarray o;
  int l;
  jdouble *ap;
  JNIEnv *env=getJNIEnv();

  profStart();
  if (e==R_NilValue) return e;
  if (TYPEOF(e)==EXTPTRSXP)
    o=(jobject)EXTPTR_PTR(e);
  else
    error_return("RgetDoubleArrayCont: invalid object parameter");
  rjprintf(" jarray %d\n",o);
  if (!o) return R_NilValue;
  l=(int)(*env)->GetArrayLength(env, o);
  rjprintf("convert double array of length %d\n",l);
  if (l<0) return R_NilValue;
  ap=(jdouble*)(*env)->GetDoubleArrayElements(env, o, 0);
  if (!ap)
    error_return("RgetDoubleArrayCont: can't fetch array contents");
  PROTECT(ar=allocVector(REALSXP,l));
  if (l>0) memcpy(REAL(ar),ap,sizeof(jdouble)*l);
  UNPROTECT(1);
  (*env)->ReleaseDoubleArrayElements(env, o, ap, 0);
  profReport("RgetDoubleArrayCont[%d]:",o);
  return ar;
}

/** get contents of the long array object (int) */
SEXP RgetLongArrayCont(SEXP par) {
  SEXP e=CAR(CDR(par));
  SEXP ar;
  jarray o;
  int l;
  jlong *ap;
  JNIEnv *env=getJNIEnv();

  profStart();
  if (e==R_NilValue) return e;
  if (TYPEOF(e)==EXTPTRSXP)
    o=(jobject)EXTPTR_PTR(e);
  else
    error_return("RgetLongArrayCont: invalid object parameter");
  rjprintf(" jarray %d\n",o);
  if (!o) return R_NilValue;
  l=(int)(*env)->GetArrayLength(env, o);
  rjprintf("convert long array of length %d\n",l);
  if (l<0) return R_NilValue;
  ap=(jlong*)(*env)->GetLongArrayElements(env, o, 0);
  if (!ap)
    error_return("RgetLongArrayCont: can't fetch array contents");
  PROTECT(ar=allocVector(REALSXP,l));
  { /* long must be coerced into double .. we use just a cast for each element, bad idea? */
    int i=0;
    while (i<l) { REAL(ar)[i]=(double)ap[i]; i++; }
  }
  UNPROTECT(1);
  (*env)->ReleaseLongArrayElements(env, o, ap, 0);
  profReport("RgetLongArrayCont[%d]:",o);
  return ar;
}

/** call specified non-static method on an object
   object (int), return signature (string), method name (string) [, ..parameters ...]
   arrays and objects are returned as IDs (hence not evaluated)
*/
SEXP RcallMethod(SEXP par) {
  SEXP p=par, e;
  char sig[256];
  jvalue jpar[32];
  jobject o;
  char *retsig, *mnam;
  jmethodID mid=0;
  jclass cls;
  JNIEnv *env=getJNIEnv();
  
  profStart();
  p=CDR(p); e=CAR(p); p=CDR(p);
  if (e==R_NilValue) 
    error_return("RcallMethod: call on a NULL object");
  if (TYPEOF(e)==EXTPTRSXP)
    o=(jobject)EXTPTR_PTR(e);
  else
    error_return("RcallMethod: invalid object parameter");
  if (!o)
    error_return("RcallMethod: attempt to call a method of a NULL object.");
#ifdef RJAVA_DEBUG
  {
    SEXP de=CAR(CDR(p));
    rjprintf("RcallMethod (env=%x):\n",env);
    if (TYPEOF(de)==STRSXP && LENGTH(de)>0)
      rjprintf(" method to call: %s on object 0x%x\n",CHAR(STRING_ELT(de,0)),o);
  }
#endif
  cls=(*env)->GetObjectClass(env,o);
  if (!cls)
    error_return("RcallMethod: cannot determine object class");
#ifdef RJAVA_DEBUG
  rjprintf(" class: "); printObject(env, cls);
#endif
  e=CAR(p); p=CDR(p);
  if (TYPEOF(e)==STRSXP && LENGTH(e)==1) { /* signature */
    retsig=CHAR(STRING_ELT(e,0));
    /*
      } else if (inherits(e, "jobjRef")) { method object 
    SEXP mexp = GET_SLOT(e, install("jobj"));
    jobject mobj = (jobject)(INTEGER(mexp)[0]);
    rjprintf(" signature is Java object %x - using reflection\n", mobj);
    mid = (*env)->FromReflectedMethod(*env, jobject mobj);
    retsig = getReturnSigFromMethodObject(mobj);
    */
  } else error_return("RcallMethod: invalid return signature parameter");
    
  e=CAR(p); p=CDR(p);
  if (TYPEOF(e)!=STRSXP || LENGTH(e)!=1)
    error_return("RcallMethod: invalid method name");
  mnam=CHAR(STRING_ELT(e,0));
  strcpy(sig,"(");
  Rpar2jvalue(env,p,jpar,sig,32,256);
  strcat(sig,")");
  strcat(sig,retsig);
  rjprintf(" Method %s signature is %s\n",mnam,sig);
  mid=(*env)->GetMethodID(env,cls,mnam,sig);
  if (!mid)
    error_return("RcallMethod: method not found");
#if (RJ_PROFILE>1)
  profReport("Found CID/MID for %s %s:",mnam,sig);
#endif
  if (*retsig=='V') {
    (*env)->CallVoidMethodA(env,o,mid,jpar);
    profReport("Method %s returned:",mnam);
    return R_NilValue;
  }
  if (*retsig=='I') {
    int r=(*env)->CallIntMethodA(env,o,mid,jpar);
    PROTECT(e=allocVector(INTSXP, 1));
    INTEGER(e)[0]=r;
    UNPROTECT(1);
    profReport("Method %s returned:",mnam);
    return e;
  }
  if (*retsig=='J') { 
    jlong r=(*env)->CallLongMethodA(env,o,mid,jpar);
    PROTECT(e=allocVector(REALSXP, 1));
    REAL(e)[0]=(double)r;
    UNPROTECT(1);
    profReport("Method %s returned:",mnam);
    return e;
  }
  if (*retsig=='Z') {
    jboolean r=(*env)->CallBooleanMethodA(env,o,mid,jpar);
    PROTECT(e=allocVector(LGLSXP, 1));
    LOGICAL(e)[0]=(r)?1:0;
    UNPROTECT(1);
    profReport("Method %s returned:",mnam);
    return e;
  }
  if (*retsig=='D') {
    double r=(*env)->CallDoubleMethodA(env,o,mid,jpar);
    PROTECT(e=allocVector(REALSXP, 1));
    REAL(e)[0]=r;
    UNPROTECT(1);
    profReport("Method %s returned:",mnam);
    return e;
  }
  if (*retsig=='L' || *retsig=='[') {
    jobject gr;
    jobject r=(*env)->CallObjectMethodA(env,o,mid,jpar);
    if (!r) {
      profReport("Method %s returned NULL:",mnam);
      return R_NilValue;
    }
    gr=r;
    if (r) {
      gr=makeGlobal(env, r);
      if (gr)
	releaseObject(env, r);
    }
    e=j2SEXP(gr);
    profReport("Method %s returned [g.ref=%x]:",mnam, gr);
    return e;
  }
  profReport("Method %s has an unknown signature, not called:",mnam);
  return R_NilValue;
}

/** like RcallMethod but the call will be synchronized */
SEXP RcallSyncMethod(SEXP par) {
  SEXP p=par, e;
  jobject o;
  JNIEnv *env=getJNIEnv();

  p=CDR(p); e=CAR(p); p=CDR(p);
  if (e==R_NilValue) 
    error_return("RcallMethod: call on a NULL object");
  if (TYPEOF(e)==EXTPTRSXP)
    o=(jobject)EXTPTR_PTR(e);
  else
    error_return("RcallSyncMethod: invalid object parameter");
  if (!o)
    error_return("RcallSyncMethod: attempt to call a method of a NULL object.");
#ifdef RJAVA_DEBUG
  rjprintf("RcallSyncMethod:\ object: "); printObject(o);
#endif
  if ((*env)->MonitorEnter(env, o) != JNI_OK) {
    printf("Rglue.warning: couldn't get monitor on the object, running unsynchronized.\n");
    return RcallMethod(par);
  }

  e=RcallMethod(par);

  if ((*env)->MonitorExit(env, o) != JNI_OK)
    printf("Rglue.SERIOUS PROBLEM: MonitorExit failed, subsequent calls may cause a deadlock!\n");

  return e;
}


/** call specified static method of a class.
   class (string), return signature (string), method name (string) [, ..parameters ...]
   arrays and objects are returned as IDs (hence not evaluated)
*/
SEXP RcallStaticMethod(SEXP par) {
  SEXP p=par, e;
  char sig[256];
  jvalue jpar[32];
  char *cnam, *retsig, *mnam;
  jmethodID mid;
  jclass cls;
  JNIEnv *env=getJNIEnv();

  profStart();
  p=CDR(p); e=CAR(p); p=CDR(p);
  if (TYPEOF(e)!=STRSXP || LENGTH(e)!=1)
    error_return("RcallStaticMethod: invalid class parameter");
  cnam=CHAR(STRING_ELT(e,0));
#ifdef RJAVA_DEBUG
  rjprintf("class: %s\n",cnam);
#endif
  cls=getClass(env, cnam);
  if (!cls)
    error_return("RcallStaticMethod: cannot find specified class");
  e=CAR(p); p=CDR(p);
  if (TYPEOF(e)!=STRSXP || LENGTH(e)!=1)
    error_return("RcallMethod: invalid return signature parameter");
  retsig=CHAR(STRING_ELT(e,0));
  e=CAR(p); p=CDR(p);
  if (TYPEOF(e)!=STRSXP || LENGTH(e)!=1)
    error_return("RcallMethod: invalid method name");
  mnam=CHAR(STRING_ELT(e,0));
  strcpy(sig,"(");
  Rpar2jvalue(env,p,jpar,sig,32,256);
  strcat(sig,")");
  strcat(sig,retsig);
  rjprintf("Method %s signature is %s\n",mnam,sig);
  mid=(*env)->GetStaticMethodID(env,cls,mnam,sig);
  if (!mid)
    error_return("RcallStaticMethod: method not found");
#if (RJ_PROFILE>1)
  profReport("Found CID/MID for %s %s:",mnam,sig);
#endif
  if (*retsig=='V') {
    (*env)->CallStaticVoidMethodA(env,cls,mid,jpar);
    profReport("Method %s returned:",mnam);
    return R_NilValue;
  }
  if (*retsig=='I') {
    int r=(*env)->CallStaticIntMethodA(env,cls,mid,jpar);
    PROTECT(e=allocVector(INTSXP, 1));
    INTEGER(e)[0]=r;
    UNPROTECT(1);
    profReport("Method %s returned:",mnam);
    return e;
  }
  if (*retsig=='J') {
    jlong r=(*env)->CallStaticLongMethodA(env,cls,mid,jpar);
    PROTECT(e=allocVector(REALSXP, 1));
    REAL(e)[0]=(double)r;
    UNPROTECT(1);
    profReport("Method %s returned:",mnam);
    return e;
  }
  if (*retsig=='Z') {
    jboolean r=(*env)->CallStaticBooleanMethodA(env,cls,mid,jpar);
    PROTECT(e=allocVector(LGLSXP, 1));
    LOGICAL(e)[0]=(r)?1:0;
    UNPROTECT(1);
    profReport("Method %s returned:",mnam);
    return e;
  }
  if (*retsig=='D') {
    double r=(*env)->CallStaticDoubleMethodA(env,cls,mid,jpar);
    PROTECT(e=allocVector(REALSXP, 1));
    REAL(e)[0]=r;
    UNPROTECT(1);
    profReport("Method %s returned:",mnam);
    return e;
  }
  if (*retsig=='L' || *retsig=='[') {
    jobject gr;
    jobject r=(*env)->CallStaticObjectMethodA(env,cls,mid,jpar);
    gr=r;
    if (r) {
      gr=makeGlobal(env, r);
      if (gr)
	releaseObject(env, r);
    }
    profReport("Method %s returned:",mnam);
    return j2SEXP(gr);
  }
  profReport("Method %s has an unknown sigrature, not called:",mnam);
  return R_NilValue;
}

/** get value of a non-static field of an object
    object (int), return signature (string), field name (string)
    arrays and objects are returned as IDs (hence not evaluated)
*/
SEXP RgetField(SEXP par) {
  SEXP p=par, e;
  jobject o;
  char *retsig, *mnam;
  jfieldID mid;
  jclass cls;
  JNIEnv *env=getJNIEnv();

  p=CDR(p); e=CAR(p); p=CDR(p);
  if (e==R_NilValue) return R_NilValue;
  if (TYPEOF(e)==EXTPTRSXP)
    o=(jobject)EXTPTR_PTR(e);
  else
    error_return("RgetField: invalid object parameter");
  if (!o)
    error_return("RgetField: attempt to get field of a NULL object");
#ifdef RJAVA_DEBUG
  rjprintf("object: "); printObject(env, o);
#endif
  cls=(*env)->GetObjectClass(env,o);
  if (!cls)
    error_return("RgetField: cannot determine object class");
#ifdef RJAVA_DEBUG
  rjprintf("class: "); printObject(env, cls);
#endif
  e=CAR(p); p=CDR(p);
  if (TYPEOF(e)!=STRSXP || LENGTH(e)!=1)
    error_return("RgetField: invalid return signature parameter");
  retsig=CHAR(STRING_ELT(e,0));
  e=CAR(p); p=CDR(p);
  if (TYPEOF(e)!=STRSXP || LENGTH(e)!=1)
    error_return("RgetField: invalid field name");
  mnam=CHAR(STRING_ELT(e,0));
  rjprintf("field %s signature is %s\n",mnam,retsig);
  mid=(*env)->GetFieldID(env,cls,mnam,retsig);
  if (!mid)
    error_return("RgetField: field not found");
  if (*retsig=='I') {
    int r=(*env)->GetIntField(env,o,mid);
    PROTECT(e=allocVector(INTSXP, 1));
    INTEGER(e)[0]=r;
    UNPROTECT(1);
    return e;
  }
  if (*retsig=='J') {
    jlong r=(*env)->GetLongField(env,o,mid); /* FIXME: jlong=int ?? */
    PROTECT(e=allocVector(REALSXP, 1));
    REAL(e)[0]=(double)r;
    UNPROTECT(1);
    return e;
  }
  if (*retsig=='Z') {
    jboolean r=(*env)->GetBooleanField(env,o,mid);
    PROTECT(e=allocVector(LGLSXP, 1));
    LOGICAL(e)[0]=(r)?1:0;
    UNPROTECT(1);
    return e;
  }
  if (*retsig=='D') {
    double r=(*env)->GetDoubleField(env,o,mid);
    PROTECT(e=allocVector(REALSXP, 1));
    REAL(e)[0]=r;
    UNPROTECT(1);
    return e;
  }
  if (*retsig=='L' || *retsig=='[') {
    jobject gr;
    jobject r=(*env)->GetObjectField(env,o,mid);
    gr=r;
    /* unlike method results: fields, we don't make them global
    if (r) {
      gr=makeGlobal(r);
      if (gr)
	releaseObject(r);
    }
    */
    return j2SEXP(gr);
  }
  return R_NilValue;
}

/** create new object.
    fully-qualified class in JNI notation (string) [, constructor parameters] */
SEXP RcreateObject(SEXP par) {
  SEXP p=par;
  SEXP e;
  char *class;
  char sig[256];
  jvalue jpar[32];
  jobject o,go;
  JNIEnv *env=getJNIEnv();

  if (TYPEOF(p)!=LISTSXP) {
    rjprintf("Parameter list expected but got type %d.\n",TYPEOF(p));
    error_return("RcreateObject: invalid parameter");
  }

  p=CDR(p); /* skip first parameter which is the function name */
  e=CAR(p); /* second is the class name */
  if (TYPEOF(e)!=STRSXP || LENGTH(e)!=1)
    error_return("RcreateObject: invalid class name");
  class=CHAR(STRING_ELT(e,0));
  rjprintf("new %s(...)\n",class);
  strcpy(sig,"(");
  p=CDR(p);
  Rpar2jvalue(env,p,jpar,sig,32,256);
  strcat(sig,")V");
  rjprintf("Constructor signature is %s\n",sig);
  o=createObject(env,class,sig,jpar);
  if (!o) return R_NilValue;
  go=makeGlobal(env,o);
  if (go)
    releaseObject(env,o);
  else
    go=o;

#ifdef RJ_DEBUG
  {
    jstring s=callToString(go);
    const char *c=(*env)->GetStringUTFChars(env, s, 0);
    rjprintf("New Java object [%s] reference %lx\n", c, (long)go);
    (*env)->ReleaseStringUTFChars(env, s, c);
  }
#endif
  
  return j2SEXP(go);
}

SEXP new_jarrayRef(jobject a, char *sig) {
  /* it is too tedious to try to do this in C, so we use 'new' R function instead */
  SEXP oo = eval(LCONS(install("new"),LCONS(mkString("jarrayRef"),R_NilValue)), R_GlobalEnv);
  /* .. and set the slots in C .. */
  if (inherits(oo, "jarrayRef")) {
    SET_SLOT(oo, install("jobj"), j2SEXP(a));
    SET_SLOT(oo, install("jclass"), mkString("java/lang/Object"));
    SET_SLOT(oo, install("jsig"), mkString(sig));
    return oo;
  }
  return R_NilValue;
}

SEXP RcreateArray(SEXP ar) {
  JNIEnv *env=getJNIEnv();
  
  if (ar==R_NilValue) return R_NilValue;
  switch(TYPEOF(ar)) {
  case INTSXP:
    {
      if (inherits(ar, "jchar")) {
	jcharArray a = newCharArrayI(env, INTEGER(ar), LENGTH(ar));
	if (!a) return R_NilValue;
	return new_jarrayRef(a, "[C");
      } else {
	jintArray a = newIntArray(env, INTEGER(ar), LENGTH(ar));
	if (!a) return R_NilValue;
	return new_jarrayRef(a, "[I");
      }
    }
  case REALSXP:
    {
      if (inherits(ar, "jfloat")) {
	jfloatArray a = newFloatArrayD(env, REAL(ar), LENGTH(ar));
	if (!a) return R_NilValue;
	return new_jarrayRef(a, "[F");
      } else {
	jdoubleArray a = newDoubleArray(env, REAL(ar), LENGTH(ar));
	if (!a) return R_NilValue;
	return new_jarrayRef(a, "[D");
      }
    }
  case STRSXP:
    {
      jobjectArray a = (*env)->NewObjectArray(env, LENGTH(ar), javaStringClass, 0);
      int i=0;
      if (!a) return R_NilValue;
      while (i<LENGTH(ar)) {
	(*env)->SetObjectArrayElement(env, a, i, (*env)->NewStringUTF(env, CHAR(STRING_ELT(ar, i))));
	i++;
      }
      return new_jarrayRef(a, "[Ljava/lang/String;");
    }
  case LGLSXP:
    {
      /* ASSUMPTION: LOGICAL()=int* */
      jbooleanArray a = newBooleanArrayI(env, LOGICAL(ar), LENGTH(ar));
      if (!a) return R_NilValue;
      return new_jarrayRef(a, "[Z");
    }
  case VECSXP:
    {
      int i=0;
      while (i<LENGTH(ar)) {
	SEXP e = VECTOR_ELT(ar, i);
	if (e!=R_NilValue && !inherits(e, "jobjRef") && !inherits(e, "jarrayRef")) error("Cannot create a Java array from a list that contains anything other than Java object references.");
	i++;
      }
      {
	jobjectArray a = (*env)->NewObjectArray(env, LENGTH(ar), javaObjectClass, 0);
	i=0;
	if (!a) return R_NilValue;
	while (i<LENGTH(ar)) {
	  SEXP e = VECTOR_ELT(ar, i);
	  jobject o = 0;
	  if (e != R_NilValue) {
	    SEXP sref=GET_SLOT(e, install("jobj"));
	    if (sref && TYPEOF(sref)==EXTPTRSXP)
	      o=(jobject)EXTPTR_PTR(sref);
	  }	  
	  (*env)->SetObjectArrayElement(env, a, i, o);
	  i++;
	}
	return new_jarrayRef(a, "[Ljava/lang/Object;");
      }
    }
  case RAWSXP:
    {
      jbyteArray a = newByteArray(env, RAW(ar), LENGTH(ar));
      if (!a) return R_NilValue;
      return new_jarrayRef(a, "[B");
    }
  }
  error("Unsupported type to create Java array from.");
  return R_NilValue;
}

/* compares two references */
SEXP RidenticalRef(SEXP ref1, SEXP ref2) {
  SEXP r;
  if (TYPEOF(ref1)!=EXTPTRSXP || TYPEOF(ref2)!=EXTPTRSXP) return R_NilValue;
  r=allocVector(LGLSXP,1);
  LOGICAL(r)[0]=(R_ExternalPtrAddr(ref1)==R_ExternalPtrAddr(ref2));
  return r;
}

/** jobjRefInt object : string */
SEXP RfreeObject(SEXP par) {
  SEXP p,e;
  jobject o;
  JNIEnv *env=getJNIEnv();

  p=CDR(par); e=CAR(p); p=CDR(p);
  if (e==R_NilValue) return e;
  if (TYPEOF(e)==EXTPTRSXP)
    o=(jobject)EXTPTR_PTR(e);
  else
    error_return("RfreeObject: invalid object parameter");
  releaseGlobal(env, o);
  return R_NilValue;
}

/** create a NULL external reference */
SEXP RgetNullReference() {
  return R_MakeExternalPtr(0, R_NilValue, R_NilValue);
}
