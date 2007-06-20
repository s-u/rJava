#include "rJava.h"
#include <R.h>
#include <Rdefines.h>
#include <R_ext/Parse.h>
#include <R_ext/Print.h>

#include <stdarg.h>

/* max supported # of parameters to Java methdos */
#ifndef maxJavaPars
#define maxJavaPars 32
#endif

/* pre-2.4 have no S4SXP but used VECSXP instead */
#ifndef S4SXP
#define S4SXP VECSXP
#endif

#ifdef ENABLE_JRICB
#define BEGIN_RJAVA_CALL { int save_in_RJava = RJava_has_control; RJava_has_control=1; {
#define END_RJAVA_CALL }; RJava_has_control = save_in_RJava; }
#else
#define BEGIN_RJAVA_CALL {
#define END_RJAVA_CALL };
#endif

/** returns TRUE if JRI has callback support compiled in or FALSE otherwise */
SEXP RJava_has_jri_cb() {
  SEXP r = allocVector(LGLSXP, 1);
#ifdef ENABLE_JRICB
  LOGICAL(r)[0] = 1;
#else
  LOGICAL(r)[0] = 0;
#endif
  return r;
} 

static SEXP new_jobjRef(JNIEnv *env, jobject o, const char *klass);

/* debugging output (enable with -DRJ_DEBUG) */
#ifdef RJ_DEBUG
void rjprintf(char *fmt, ...) {
  va_list v;
  va_start(v,fmt);
  Rvprintf(fmt,v);
  va_end(v);
}
/* we can't assume ISO C99 (variadic macros), so we have to use one more level of wrappers */
#define _dbg(X) X
#else
#define _dbg(X)
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

static long profilerTime;

#define profStart() profilerTime=time_ms()
void profReport(char *fmt, ...) {
  long npt=time_ms();
  va_list v;
  va_start(v,fmt);
  Rvprintf(fmt,v);
  va_end(v);
  Rprintf(" %ld ms\n",npt-profilerTime);
  profilerTime=npt;
}
#define _prof(X) X
#else
#define profStart()
#define _prof(X)
#endif

jstring callToString(JNIEnv *env, jobject o);

void JRefObjectFinalizer(SEXP ref) {
  if (TYPEOF(ref)==EXTPTRSXP) {
    JNIEnv *env=getJNIEnv();
    jobject o = R_ExternalPtrAddr(ref);

#ifdef RJ_DEBUG
    {
      jstring s=callToString(env, o);
      const char *c="???";
      if (s) c=(*env)->GetStringUTFChars(env, s, 0);
      _dbg(rjprintf("Finalizer releases Java object [%s] reference %lx (SEXP@%lx)\n", c, (long)o, (long)ref));
      if (s) {
	(*env)->ReleaseStringUTFChars(env, s, c);
	releaseObject(env, s);
      }
    }
#endif

    if (env && o) {
      /* _dbg(rjprintf("  finalizer releases global reference %lx\n", (long)o);) */
      _mp(MEM_PROF_OUT("R %08x FIN\n", (int)o))
      releaseGlobal(env, o);
    }
  }
}

/* jobject to SEXP encoding - 0.2 and earlier use INTSXP */
SEXP j2SEXP(JNIEnv *env, jobject o, int releaseLocal) {
  if (!env) error("Invalid Java environment in j2SEXP");
  if (o) {
    jobject go = makeGlobal(env, o);
    _mp(MEM_PROF_OUT("R %08x RNEW %08x\n", (int) go, (int) o))
    if (!go)
      error("Failed to create a global reference in Java.");
    _dbg(rjprintf(" j2SEXP: %lx -> %lx (release=%d)\n", (long)o, (long)go, releaseLocal));
    if (releaseLocal)
      releaseObject(env, o);
    o=go;
  }
  
  {
    SEXP xp = R_MakeExternalPtr(o, R_NilValue, R_NilValue);

#ifdef RJ_DEBUG
    {
      JNIEnv *env=getJNIEnv();
      jstring s=callToString(env, o);
      const char *c="???";
      if (s) c=(*env)->GetStringUTFChars(env, s, 0);
      _dbg(rjprintf("New Java object [%s] reference %lx (SEXP@%lx)\n", c, (long)o, (long)xp));
      if (s) {
	(*env)->ReleaseStringUTFChars(env, s, c);
	releaseObject(env, s);
      }
    }
#endif

    R_RegisterCFinalizerEx(xp, JRefObjectFinalizer, TRUE);
    return xp;
  }
}

static int    jvm_opts=0;
static char **jvm_optv=0;

#ifdef THREADS
#include <pthread.h>

#ifdef XXDARWIN
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFRunLoop.h>
#endif

pthread_t initJVMpt;
pthread_mutex_t initMutex = PTHREAD_MUTEX_INITIALIZER;

int thInitResult = 0;
int initAWT = 0;

void *initJVMthread(void *classpath)
{
  int ws;
  jclass c;
  JNIEnv *lenv;

  thInitResult=initJVM((char*)classpath, jvm_opts, jvm_optv);
  if (thInitResult) return 0;

  init_rJava();

  lenv = eenv; /* we make a local copy before unlocking just in case
		  someone messes with it before we can use it */

  _dbg(rjprintf("initJVMthread: unlocking\n"));
  pthread_mutex_unlock(&initMutex);

  if (initAWT) {
    _dbg(rjprintf("initJVMthread: get AWT class\n"));
    /* we are still on the same thread, so we can safely use eenv */
    c = (*lenv)->FindClass(lenv, "java/awt/Frame");
  }

  _dbg(rjprintf("initJVMthread: returning from the init thread.\n"));
  return 0;
}

#endif

/** RinitJVM(classpath)
    initializes JVM with the specified class path */
SEXP RinitJVM(SEXP par)
{
  const char *c=0;
  SEXP e=CADR(par);
  int r=0;
  JavaVM *jvms[32];
  jsize vms=0;
  
  jvm_opts=0;
  jvm_optv=0;
  
  if (TYPEOF(e)==STRSXP && LENGTH(e)>0)
	  c=CHAR(STRING_ELT(e,0));

  e = CADDR(par);
  if (TYPEOF(e)==STRSXP && LENGTH(e)>0) {
	  int len = LENGTH(e);
	  jvm_optv=(char**)malloc(sizeof(char*)*len);
	  while (jvm_opts < len) {
		  jvm_optv[jvm_opts] = strdup(CHAR(STRING_ELT(e, jvm_opts)));
		  jvm_opts++;
	  }
  }
  
  r=JNI_GetCreatedJavaVMs(jvms, 32, &vms);
  if (r) {
    Rf_error("JNI_GetCreatedJavaVMs returned %d\n", r);
  } else {
    if (vms>0) {
      int i=0;
      _dbg(rjprintf("RinitJVM: Total %d JVMs found. Trying to attach the current thread.\n", (int)vms));
      while (i<vms) {
	if (jvms[i]) {
	  if (!(*jvms[i])->AttachCurrentThread(jvms[i], (void**)&eenv, NULL)) {
            _dbg(rjprintf("RinitJVM: Attached to existing JVM #%d.\n", i+1));
	    break;
	  }
	}
	i++;
      }
      if (i==vms) Rf_error("Failed to attach to any existing JVM.");
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
  if (getenv("R_GUI_APP_VERSION") || getenv("RJAVA_INIT_AWT"))
    initAWT=1;

  _dbg(rjprintf("RinitJVM(threads): launching thread\n"));
  pthread_mutex_lock(&initMutex);
  pthread_create(&initJVMpt, 0, initJVMthread, c);
  _dbg(rjprintf("RinitJVM(threads): waiting for mutex\n"));
  pthread_mutex_lock(&initMutex);
  pthread_mutex_unlock(&initMutex);
  /* pthread_join(initJVMpt, 0); */
  _dbg(rjprintf("RinitJVM(threads): attach\n"));
  /* since JVM was initialized by another thread, we need to attach ourselves */
  (*jvm)->AttachCurrentThread(jvm, (void**)&eenv, NULL);
  _dbg(rjprintf("RinitJVM(threads): done.\n"));
  r = thInitResult;
#else
  profStart();
  r=initJVM(c, jvm_opts, jvm_optv);
  init_rJava();
  _prof(profReport("init_rJava:"));
  _dbg(rjprintf("RinitJVM(non-threaded): initJVM returned %d\n", r));
#endif
  if (jvm_optv) free(jvm_optv);
  jvm_opts=0;
  PROTECT(e=allocVector(INTSXP,1));
  INTEGER(e)[0]=r;
  UNPROTECT(1);
  return e;
}

#define addtmpo(T, X) { jobject _o = X; if (_o) { _dbg(rjprintf(" parameter to release later: %lx\n", (unsigned long) _o)); *T=_o; T++;} }
#define fintmpo(T) { *T = 0; }


/** converts parameters in SEXP list to jpar and sig.
    strcat is used on sig, hence sig must be a valid string already
    since 0.4-4 we ignore named arguments in par
    Note: maxsig is never used and thus the sig buffer could overflow
*/
static int Rpar2jvalue(JNIEnv *env, SEXP par, jvalue *jpar, char *sig, int maxpars, int maxsig, jobject *tmpo) {
  SEXP p=par;
  SEXP e;
  int jvpos=0;
  int i=0;

  while (p && TYPEOF(p)==LISTSXP && (e=CAR(p))) {
    /* skip all named arguments */
    if (TAG(p) && TAG(p)!=R_NilValue) { p=CDR(p); continue; };
    
    _dbg(rjprintf("Rpar2jvalue: par %d type %d\n",i,TYPEOF(e)));
    if (TYPEOF(e)==STRSXP) {
      _dbg(rjprintf(" string vector of length %d\n",LENGTH(e)));
      if (LENGTH(e)==1) {
	strcat(sig,"Ljava/lang/String;");
	addtmpo(tmpo, jpar[jvpos++].l=newString(env, CHAR(STRING_ELT(e,0))));
      } else {
	int j=0;
	jobjectArray sa=(*env)->NewObjectArray(env, LENGTH(e), javaStringClass, 0);
	_mp(MEM_PROF_OUT("  %08x LNEW string[%d]\n", (int) sa, LENGTH(e)))
	if (!sa) {
	  fintmpo(tmpo);
	  error("unable to create string array.");
	  return -1;
	}
	addtmpo(tmpo, sa);
	while (j<LENGTH(e)) {
	  jobject s=newString(env, CHAR(STRING_ELT(e,j)));
	  _dbg(rjprintf (" [%d] \"%s\"\n",j,CHAR(STRING_ELT(e,j))));
	  (*env)->SetObjectArrayElement(env, sa, j, s);
	  if (s) releaseObject(env, s);
	  j++;
	}
	jpar[jvpos++].l=sa;
	strcat(sig,"[Ljava/lang/String;");
      }
    } else if (TYPEOF(e)==INTSXP) {
      _dbg(rjprintf(" integer vector of length %d\n",LENGTH(e)));
      if (LENGTH(e)==1) {
	if (inherits(e, "jbyte")) {
	  _dbg(rjprintf("  (actually a single byte 0x%x)\n", INTEGER(e)[0]));
	  jpar[jvpos++].b=(jbyte)(INTEGER(e)[0]);
	  strcat(sig,"B");
	} else if (inherits(e, "jchar")) {
	  _dbg(rjprintf("  (actually a single character 0x%x)\n", INTEGER(e)[0]));
	  jpar[jvpos++].c=(jchar)(INTEGER(e)[0]);
	  strcat(sig,"C");
	} else if (inherits(e, "jshort")) {
	  _dbg(rjprintf("  (actually a single short 0x%x)\n", INTEGER(e)[0]));
	  jpar[jvpos++].s=(jshort)(INTEGER(e)[0]);
	  strcat(sig,"S");
	} else {
	  strcat(sig,"I");
	  jpar[jvpos++].i=(jint)(INTEGER(e)[0]);
	  _dbg(rjprintf("  single int orig=%d, jarg=%d [jvpos=%d]\n",
		   (INTEGER(e)[0]),
		   jpar[jvpos-1],
		   jvpos));
	}
      } else {
	if (inherits(e, "jbyte")) {
	  strcat(sig,"[B");
	  addtmpo(tmpo, jpar[jvpos++].l=newByteArrayI(env, INTEGER(e), LENGTH(e)));
	} else if (inherits(e, "jchar")) {
	  strcat(sig,"[C");
	  addtmpo(tmpo, jpar[jvpos++].l=newCharArrayI(env, INTEGER(e), LENGTH(e)));
	} else if (inherits(e, "jshort")) {
	  strcat(sig,"[S");
	  addtmpo(tmpo, jpar[jvpos++].l=newShortArrayI(env, INTEGER(e), LENGTH(e)));
	} else {
	  strcat(sig,"[I");
	  addtmpo(tmpo, jpar[jvpos++].l=newIntArray(env, INTEGER(e), LENGTH(e)));
	}
      }
    } else if (TYPEOF(e)==REALSXP) {
      if (inherits(e, "jfloat")) {
	_dbg(rjprintf(" jfloat vector of length %d\n", LENGTH(e)));
	if (LENGTH(e)==1) {
	  strcat(sig,"F");
	  jpar[jvpos++].f=(jfloat)(REAL(e)[0]);
	} else {
	  strcat(sig,"[F");
	  addtmpo(tmpo, jpar[jvpos++].l=newFloatArrayD(env, REAL(e),LENGTH(e)));
	}
      } else if (inherits(e, "jlong")) {
	_dbg(rjprintf(" jlong vector of length %d\n", LENGTH(e)));
	if (LENGTH(e)==1) {
	  strcat(sig,"J");
	  jpar[jvpos++].j=(jlong)(REAL(e)[0]);
	} else {
	  strcat(sig,"[J");
	  addtmpo(tmpo, jpar[jvpos++].l=newLongArrayD(env, REAL(e),LENGTH(e)));
	}
      } else {
	_dbg(rjprintf(" real vector of length %d\n",LENGTH(e)));
	if (LENGTH(e)==1) {
	  strcat(sig,"D");
	  jpar[jvpos++].d=(jdouble)(REAL(e)[0]);
	} else {
	  strcat(sig,"[D");
	  addtmpo(tmpo, jpar[jvpos++].l=newDoubleArray(env, REAL(e),LENGTH(e)));
	}
      }
    } else if (TYPEOF(e)==LGLSXP) {
      _dbg(rjprintf(" logical vector of length %d\n",LENGTH(e)));
      if (LENGTH(e)==1) {
	strcat(sig,"Z");
	jpar[jvpos++].z=(jboolean)(LOGICAL(e)[0]);
      } else {
	strcat(sig,"[Z");
	addtmpo(tmpo, jpar[jvpos++].l=newBooleanArrayI(env, LOGICAL(e),LENGTH(e)));
      }
    } else if (TYPEOF(e)==VECSXP || TYPEOF(e)==S4SXP) {
      _dbg(rjprintf(" generic vector of length %d\n", LENGTH(e)));
      if (inherits(e,"jobjRef")||inherits(e,"jarrayRef")) {
	jobject o=(jobject)0;
	const char *jc=0;
	SEXP n=getAttrib(e, R_NamesSymbol);
	if (TYPEOF(n)!=STRSXP) n=0;
	_dbg(rjprintf(" which is in fact a Java object reference\n"));
	if (TYPEOF(e)==VECSXP && LENGTH(e)>1) { /* old objects were lists */
	  fintmpo(tmpo);
	  error("Old, unsupported S3 Java object encountered.");
	} else { /* new objects are S4 objects */
	  SEXP sref, sclass;
	  sref=GET_SLOT(e, install("jobj"));
	  if (sref && TYPEOF(sref)==EXTPTRSXP) {
	    jverify(sref);
	    o = (jobject)EXTPTR_PTR(sref);
	  } else /* if jobj is anything else, assume NULL ptr */
	    o=(jobject)0;
	  sclass=GET_SLOT(e, install("jclass"));
	  if (sclass && TYPEOF(sclass)==STRSXP && LENGTH(sclass)==1)
	    jc=CHAR(STRING_ELT(sclass,0));
	  if (inherits(e, "jarrayRef") && jc && !*jc) {
	    /* if it's jarrayRef with jclass "" then it's an uncast array - use sig instead */
	    sclass=GET_SLOT(e, install("jsig"));
	    if (sclass && TYPEOF(sclass)==STRSXP && LENGTH(sclass)==1)
	      jc=CHAR(STRING_ELT(sclass,0));
	  }
	}
	if (jc) {
	  if (*jc!='[') { /* not an array, we assume it's an object of that class */
	    strcat(sig,"L"); strcat(sig,jc); strcat(sig,";");
	  } else /* array signature is passed as-is */
	    strcat(sig,jc);
	} else
	  strcat(sig,"Ljava/lang/Object;");
	jpar[jvpos++].l=o;
      } else {
	_dbg(rjprintf(" (ignoring)\n"));
      }
    }
    i++;
    p=CDR(p);
    if (jvpos >= maxpars) break;
  }
  fintmpo(tmpo);
  return jvpos;
}

/** free parameters that were temporarily allocated */
static void Rfreejpars(JNIEnv *env, jobject *tmpo) {
  if (!tmpo) return;
  while (*tmpo) {
    _dbg(rjprintf("Rfreepars: releasing %lx\n", (unsigned long) *tmpo));
    releaseObject(env, *tmpo);
    tmpo++;
  }
}

/** map one parameter into jvalue and determine its signature */
static jvalue R1par2jvalue(JNIEnv *env, SEXP par, char *sig, jobject *otr) {
  jobject tmpo[4] = {0, 0};
  jvalue v[4];
  int p = Rpar2jvalue(env, CONS(par, R_NilValue), v, sig, 2, 64, tmpo);
  /* this should never happen, but just in case - we can only assume responsibility for one value ... */
  if (p != 1 || (tmpo[0] && tmpo[1])) {
    Rfreejpars(env, tmpo);
    error("invalid parameter");
  }
  *otr = *tmpo;
  return *v;
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
  if (TYPEOF(e)==EXTPTRSXP) {
    jverify(e);
    s=(jstring)EXTPTR_PTR(e);
  } else
    error("invalid object parameter");
  if (!s) return R_NilValue;
  c=(*env)->GetStringUTFChars(env, s, 0);
  if (!c)
    error("cannot retrieve string content");
  r = mkString(c);
  (*env)->ReleaseStringUTFChars(env, s, c);
  _prof(profReport("RgetStringValue:"));
  return r;
}

/** calls .toString() of the object and returns the corresponding string java object */
jstring callToString(JNIEnv *env, jobject o) {
  jclass cls;
  jmethodID mid;
  jstring s;

  if (!o) { _dbg(rjprintf("callToString: invoked on a NULL object\n")); return 0; }
  cls=objectClass(env,o);
  if (!cls) {
    _dbg(rjprintf("callToString: can't determine class of the object\n"));
    releaseObject(env, cls);
    checkExceptionsX(env, 1);
    return 0;
  }
  mid=(*env)->GetMethodID(env, cls, "toString", "()Ljava/lang/String;");
  if (!mid) {
    _dbg(rjprintf("callToString: toString not found for the object\n"));
    releaseObject(env, cls);
    checkExceptionsX(env, 1);
    return 0;
  }
  BEGIN_RJAVA_CALL;
  s = (jstring)(*env)->CallObjectMethod(env, o, mid);
  END_RJAVA_CALL;
  _mp(MEM_PROF_OUT("  %08x LNEW object method toString result\n", (int) s))
  releaseObject(env, cls);
  return s;
}

/** calls .toString() on the passed object (int/extptr) and returns the string 
    value or NULL if there is no toString method */
SEXP RtoString(SEXP par) {
  SEXP p,e,r;
  jstring s;
  jobject o;
  const char *c;
  JNIEnv *env=getJNIEnv();

  p=CDR(par); e=CAR(p); p=CDR(p);
  if (e==R_NilValue) return R_NilValue;
  if (TYPEOF(e)==EXTPTRSXP) {
    jverify(e);
    o=(jobject)EXTPTR_PTR(e);
  } else
    error_return("RtoString: invalid object parameter");
  if (!o) return R_NilValue;
  s=callToString(env, o);
  if (!s) {
    return R_NilValue;
  }
  c=(*env)->GetStringUTFChars(env, s, 0);
  PROTECT(r=allocVector(STRSXP,1));
  SET_STRING_ELT(r, 0, mkChar(c));
  UNPROTECT(1);
  (*env)->ReleaseStringUTFChars(env, s, c);
  releaseObject(env, s);
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
  if (TYPEOF(e)==EXTPTRSXP) {
    jverify(e);
    o=(jobject)EXTPTR_PTR(e);
  } else
    error_return("RgetObjectArrayCont: invalid object parameter");
  _dbg(rjprintf("RgetObjectArrayCont: jarray %x\n",o));
  if (!o) return R_NilValue;
  l=(int)(*env)->GetArrayLength(env, o);
  _dbg(rjprintf(" convert object array of length %d\n",l));
  if (l<1) return R_NilValue;
  PROTECT(ar=allocVector(VECSXP,l));
  i=0;
  while (i<l) {
    jobject ae = (*env)->GetObjectArrayElement(env, o, i);
    _mp(MEM_PROF_OUT("  %08x LNEW object array element [%d]\n", (int) ae, i))
    SET_VECTOR_ELT(ar, i, j2SEXP(env, ae, 1));
    i++;
  }
  UNPROTECT(1);
  _prof(profReport("RgetObjectArrayCont[%d]:",o));
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
  if (TYPEOF(e)==EXTPTRSXP) {
    jverify(e);
    o=(jobject)EXTPTR_PTR(e);
  } else
    error_return("RgetStringArrayCont: invalid object parameter");
  _dbg(rjprintf("RgetStringArrayCont: jarray %x\n",o));
  if (!o) return R_NilValue;
  l=(int)(*env)->GetArrayLength(env, o);
  _dbg(rjprintf(" convert string array of length %d\n",l));
  if (l<0) return R_NilValue;
  PROTECT(ar=allocVector(STRSXP,l));
  i=0;
  while (i<l) {
    jobject sobj=(*env)->GetObjectArrayElement(env, o, i);
    _mp(MEM_PROF_OUT("  %08x LNEW object array element [%d]\n", (int) sobj, i))
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
    if (sobj) releaseObject(env, sobj);
    i++;
  }
  UNPROTECT(1);
  _prof(profReport("RgetStringArrayCont[%d]:",o))
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
  if (TYPEOF(e)==EXTPTRSXP) {
    jverify(e);
    o=(jobject)EXTPTR_PTR(e);
  } else
    error_return("RgetIntArrayCont: invalid object parameter");
  _dbg(rjprintf("RgetIntArrayCont: jarray %x\n",o));
  if (!o) return R_NilValue;
  l=(int)(*env)->GetArrayLength(env, o);
  _dbg(rjprintf(" convert int array of length %d\n",l));
  if (l<0) return R_NilValue;
  ap=(jint*)(*env)->GetIntArrayElements(env, o, 0);
  if (!ap)
    error_return("RgetIntArrayCont: can't fetch array contents");
  PROTECT(ar=allocVector(INTSXP,l));
  if (l>0) memcpy(INTEGER(ar),ap,sizeof(jint)*l);
  UNPROTECT(1);
  (*env)->ReleaseIntArrayElements(env, o, ap, 0);
  _prof(profReport("RgetIntArrayCont[%d]:",o));
  return ar;
}

/** get contents of the boolean array object */
SEXP RgetBoolArrayCont(SEXP par) {
  SEXP e=CAR(CDR(par));
  SEXP ar;
  jarray o;
  int l;
  jboolean *ap;
  JNIEnv *env=getJNIEnv();

  profStart();
  if (e==R_NilValue) return e;
  if (TYPEOF(e)==EXTPTRSXP) {
    jverify(e);
    o=(jobject)EXTPTR_PTR(e);
  } else
    error_return("RgetBoolArrayCont: invalid object parameter");
  _dbg(rjprintf("RgetBoolArrayCont: jarray %x\n",o));
  if (!o) return R_NilValue;
  l=(int)(*env)->GetArrayLength(env, o);
  _dbg(rjprintf(" convert bool array of length %d\n",l));
  if (l<0) return R_NilValue;
  ap=(jboolean*)(*env)->GetBooleanArrayElements(env, o, 0);
  if (!ap)
    error_return("RgetBoolArrayCont: can't fetch array contents");
  PROTECT(ar=allocVector(LGLSXP,l));
  { /* jboolean = byte, logical = int, need to convert */
    int i = 0;
    while (i < l) {
      LOGICAL(ar)[i] = ap[i];
      i++;
    }
  }
  UNPROTECT(1);
  (*env)->ReleaseBooleanArrayElements(env, o, ap, 0);
  _prof(profReport("RgetBoolArrayCont[%d]:",o));
  return ar;
}

/** get contents of the byte array object */
SEXP RgetByteArrayCont(SEXP par) {
	SEXP e=CAR(CDR(par));
	SEXP ar;
	jarray o;
	int l;
	jbyte *ap;
	JNIEnv *env=getJNIEnv();
	
	profStart();
	if (e==R_NilValue) return e;
	if (TYPEOF(e)==EXTPTRSXP) {
	  jverify(e);
	  o = (jobject)EXTPTR_PTR(e);
	} else
		error_return("RgetByteArrayCont: invalid object parameter");
	_dbg(rjprintf("RgetByteArrayCont: jarray %x\n",o));
	if (!o) return R_NilValue;
	l=(int)(*env)->GetArrayLength(env, o);
	_dbg(rjprintf(" convert byte array of length %d\n",l));
	if (l<0) return R_NilValue;
	ap=(jbyte*)(*env)->GetByteArrayElements(env, o, 0);
	if (!ap)
		error_return("RgetByteArrayCont: can't fetch array contents");
	PROTECT(ar=allocVector(RAWSXP,l));
	if (l>0) memcpy(RAW(ar),ap,l);
	UNPROTECT(1);
	(*env)->ReleaseByteArrayElements(env, o, ap, 0);
	_prof(profReport("RgetByteArrayCont[%d]:",o));
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
  if (TYPEOF(e)==EXTPTRSXP) {
    jverify(o);
    o = (jobject)EXTPTR_PTR(e);
  } else
    error_return("RgetDoubleArrayCont: invalid object parameter");
  _dbg(rjprintf("RgetDoubleArrayCont: jarray %x\n",o));
  if (!o) return R_NilValue;
  l=(int)(*env)->GetArrayLength(env, o);
  _dbg(rjprintf(" convert double array of length %d\n",l));
  if (l<0) return R_NilValue;
  ap=(jdouble*)(*env)->GetDoubleArrayElements(env, o, 0);
  if (!ap)
    error_return("RgetDoubleArrayCont: can't fetch array contents");
  PROTECT(ar=allocVector(REALSXP,l));
  if (l>0) memcpy(REAL(ar),ap,sizeof(jdouble)*l);
  UNPROTECT(1);
  (*env)->ReleaseDoubleArrayElements(env, o, ap, 0);
  _prof(profReport("RgetDoubleArrayCont[%d]:",o));
  return ar;
}

/** get contents of the float array object (double) */
SEXP RgetFloatArrayCont(SEXP par) {
  SEXP e=CAR(CDR(par));
  SEXP ar;
  jarray o;
  int l;
  jfloat *ap;
  JNIEnv *env=getJNIEnv();

  profStart();
  if (e==R_NilValue) return e;
  if (TYPEOF(e)==EXTPTRSXP) {
    jverify(e);
    o = (jobject)EXTPTR_PTR(e);
  } else
    error_return("RgetFloatArrayCont: invalid object parameter");
  _dbg(rjprintf("RgetFloatArrayCont: jarray %d\n",o));
  if (!o) return R_NilValue;
  l=(int)(*env)->GetArrayLength(env, o);
  _dbg(rjprintf(" convert float array of length %d\n",l));
  if (l<0) return R_NilValue;
  ap=(jfloat*)(*env)->GetFloatArrayElements(env, o, 0);
  if (!ap)
    error_return("RgetFloatArrayCont: can't fetch array contents");
  PROTECT(ar=allocVector(REALSXP,l));
  { /* jfloat must be coerced into double .. we use just a cast for each element */
    int i=0;
    while (i<l) { REAL(ar)[i]=(double)ap[i]; i++; }
  }
  UNPROTECT(1);
  (*env)->ReleaseFloatArrayElements(env, o, ap, 0);
  _prof(profReport("RgetFloatArrayCont[%d]:",o));
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
	if (TYPEOF(e)==EXTPTRSXP) {
	  jverify(e);
	  o=(jobject)EXTPTR_PTR(e);
	} else
	  error_return("RgetLongArrayCont: invalid object parameter");
	_dbg(rjprintf("RgetLongArrayCont: jarray %d\n",o));
	if (!o) return R_NilValue;
	l=(int)(*env)->GetArrayLength(env, o);
	_dbg(rjprintf(" convert long array of length %d\n",l));
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
	_prof(profReport("RgetLongArrayCont[%d]:",o));
	return ar;
}

/** call specified non-static method on an object
   object (int), return signature (string), method name (string) [, ..parameters ...]
   arrays and objects are returned as IDs (hence not evaluated)
*/
SEXP RcallMethod(SEXP par) {
  SEXP p = par, e;
  char sig[256];
  jvalue jpar[maxJavaPars];
  jobject tmpo[maxJavaPars+1];
  jobject o = 0;
  const char *retsig, *mnam, *clnam = 0;
  jmethodID mid = 0;
  jclass cls;
  JNIEnv *env = getJNIEnv();
  
  profStart();
  p=CDR(p); e=CAR(p); p=CDR(p);
  if (e==R_NilValue) 
    error_return("RcallMethod: call on a NULL object");
  if (TYPEOF(e)==EXTPTRSXP) {
    jverify(e);
    o = (jobject)EXTPTR_PTR(e);
  } else if (TYPEOF(e)==STRSXP && LENGTH(e)==1)
    clnam = CHAR(STRING_ELT(e, 0));
  else
    error_return("RcallMethod: invalid object parameter");
  if (!o && !clnam)
    error_return("RcallMethod: attempt to call a method of a NULL object.");
#ifdef RJ_DEBUG
  {
    SEXP de=CAR(CDR(p));
    rjprintf("RcallMethod (env=%x):\n",env);
    if (TYPEOF(de)==STRSXP && LENGTH(de)>0)
      rjprintf(" method to call: %s on object 0x%x or class %s\n",CHAR(STRING_ELT(de,0)),o,clnam);
  }
#endif
  if (clnam)
    cls = findClass(env, clnam);
  else
    cls = objectClass(env,o);
  if (!cls)
    error_return("RcallMethod: cannot determine object class");
#ifdef RJ_DEBUG
  rjprintf(" class: "); printObject(env, cls);
#endif
  e=CAR(p); p=CDR(p);
  if (TYPEOF(e)==STRSXP && LENGTH(e)==1) { /* signature */
    retsig=CHAR(STRING_ELT(e,0));
    /*
      } else if (inherits(e, "jobjRef")) { method object 
    SEXP mexp = GET_SLOT(e, install("jobj"));
    jobject mobj = (jobject)(INTEGER(mexp)[0]);
    _dbg(rjprintf(" signature is Java object %x - using reflection\n", mobj);
    mid = (*env)->FromReflectedMethod(*env, jobject mobj);
    retsig = getReturnSigFromMethodObject(mobj);
    */
  } else error_return("RcallMethod: invalid return signature parameter");
    
  e=CAR(p); p=CDR(p);
  if (TYPEOF(e)!=STRSXP || LENGTH(e)!=1)
    error_return("RcallMethod: invalid method name");
  mnam = CHAR(STRING_ELT(e,0));
  strcpy(sig,"(");
  Rpar2jvalue(env,p,jpar,sig,32,256,tmpo);
  strcat(sig,")");
  strcat(sig,retsig);
  _dbg(rjprintf(" method \"%s\" signature is %s\n",mnam,sig));
  mid=o?
    (*env)->GetMethodID(env, cls, mnam, sig):
    (*env)->GetStaticMethodID(env, cls, mnam, sig);
  if (!mid) {
    Rfreejpars(env, tmpo);
    releaseObject(env, cls);
    error("method %s with signature %s not found", mnam, sig);
  }
#if (RJ_PROFILE>1)
  profReport("Found CID/MID for %s %s:",mnam,sig);
#endif
  switch (*retsig) {
  case 'V': {
BEGIN_RJAVA_CALL
    o?
      (*env)->CallVoidMethodA(env, o, mid, jpar):
      (*env)->CallStaticVoidMethodA(env, cls, mid, jpar);
END_RJAVA_CALL
    Rfreejpars(env, tmpo);
    releaseObject(env, cls);
    _prof(profReport("Method \"%s\" returned:",mnam));
    return R_NilValue;
  }
  case 'I': {
BEGIN_RJAVA_CALL
    int r=o?
      (*env)->CallIntMethodA(env, o, mid, jpar):
      (*env)->CallStaticIntMethodA(env, cls, mid, jpar);  
    e = allocVector(INTSXP, 1);
    INTEGER(e)[0] = r;
END_RJAVA_CALL
    Rfreejpars(env, tmpo);
    releaseObject(env, cls);
    _prof(profReport("Method \"%s\" returned:",mnam));
    return e;
  }
  case 'B': {
BEGIN_RJAVA_CALL
    int r=(int) (o?
		 (*env)->CallByteMethodA(env, o, mid, jpar):
		 (*env)->CallStaticByteMethodA(env, cls, mid, jpar));
    e = allocVector(INTSXP, 1);
    INTEGER(e)[0] = r;
END_RJAVA_CALL
    Rfreejpars(env, tmpo);
    releaseObject(env, cls);
    _prof(profReport("Method \"%s\" returned:",mnam));
    return e;
  }
  case 'C': {
BEGIN_RJAVA_CALL
    int r=(int) (o?
		 (*env)->CallCharMethodA(env, o, mid, jpar):
		 (*env)->CallStaticCharMethodA(env, cls, mid, jpar));
    e = allocVector(INTSXP, 1);
    INTEGER(e)[0] = r;
END_RJAVA_CALL
    Rfreejpars(env, tmpo);
    releaseObject(env, cls);
    _prof(profReport("Method \"%s\" returned:",mnam));
    return e;
   }
 case 'J': { 
BEGIN_RJAVA_CALL
    jlong r=o?
      (*env)->CallLongMethodA(env, o, mid, jpar):
      (*env)->CallStaticLongMethodA(env, cls, mid, jpar);
    e = allocVector(REALSXP, 1);
    REAL(e)[0]=(double)r;
END_RJAVA_CALL
    Rfreejpars(env, tmpo);
    releaseObject(env, cls);
    _prof(profReport("Method \"%s\" returned:",mnam));
    return e;
 }
 case 'S': { 
BEGIN_RJAVA_CALL
    jshort r=o?
      (*env)->CallShortMethodA(env, o, mid, jpar):
      (*env)->CallStaticShortMethodA(env, cls, mid, jpar);
    e = allocVector(INTSXP, 1);
    INTEGER(e)[0]=(int)r;
END_RJAVA_CALL
    Rfreejpars(env, tmpo);
    releaseObject(env, cls);
    _prof(profReport("Method \"%s\" returned:",mnam));
    return e;
 }
 case 'Z': {
BEGIN_RJAVA_CALL
    jboolean r=o?
      (*env)->CallBooleanMethodA(env, o, mid, jpar):
      (*env)->CallStaticBooleanMethodA(env, cls, mid, jpar);
    e = allocVector(LGLSXP, 1);
    LOGICAL(e)[0]=(r)?1:0;
END_RJAVA_CALL
    Rfreejpars(env, tmpo);
    releaseObject(env, cls);
    _prof(profReport("Method \"%s\" returned:",mnam));
    return e;
 }
 case 'D': {
BEGIN_RJAVA_CALL
    double r=o?
      (*env)->CallDoubleMethodA(env, o, mid, jpar):
      (*env)->CallStaticDoubleMethodA(env, cls, mid, jpar);
    e = allocVector(REALSXP, 1);
    REAL(e)[0]=r;
END_RJAVA_CALL
    Rfreejpars(env, tmpo);
    releaseObject(env, cls);
    _prof(profReport("Method \"%s\" returned:",mnam));
    return e;
  }
 case 'F': {
BEGIN_RJAVA_CALL
  double r = (double) (o?
		      (*env)->CallFloatMethodA(env, o, mid, jpar):
		      (*env)->CallStaticFloatMethodA(env, cls, mid, jpar));
  e = allocVector(REALSXP, 1);
  REAL(e)[0] = r;
END_RJAVA_CALL
  Rfreejpars(env, tmpo);
  releaseObject(env, cls);
  _prof(profReport("Method \"%s\" returned:",mnam));
  return e;
 }
 case 'L':
 case '[': {
   jobject r;
BEGIN_RJAVA_CALL
   r = o?
     (*env)->CallObjectMethodA(env, o, mid, jpar):
     (*env)->CallStaticObjectMethodA(env, cls, mid, jpar);
END_RJAVA_CALL
    Rfreejpars(env, tmpo);
    releaseObject(env, cls);
    _mp(MEM_PROF_OUT("  %08x LNEW object method \"%s\" result\n", (int) r, mnam))
    if (!r) {
      _prof(profReport("Method \"%s\" returned NULL:",mnam));
      return R_NilValue;
    }
    e = j2SEXP(env, r, 1);
    _prof(profReport("Method \"%s\" returned",mnam));
    return e;
   }
  } /* switch */
  _prof(profReport("Method \"%s\" has an unknown signature, not called:",mnam));
  releaseObject(env, cls);
  error("unsupported/invalid mathod signature %s", retsig);
  return R_NilValue;
}

/** like RcallMethod but the call will be synchronized */
SEXP RcallSyncMethod(SEXP par) {
  SEXP p=par, e;
  jobject o;
  JNIEnv *env=getJNIEnv();

  p=CDR(p); e=CAR(p); p=CDR(p);
  if (e==R_NilValue) 
    error("RcallSyncMethod: call on a NULL object");
  if (TYPEOF(e)==EXTPTRSXP) {
    jverify(e);
    o = (jobject)EXTPTR_PTR(e);
  } else
    error("RcallSyncMethod: invalid object parameter");
  if (!o)
    error("RcallSyncMethod: attempt to call a method of a NULL object.");
#ifdef RJ_DEBUG
  rjprintf("RcallSyncMethod: object: "); printObject(env, o);
#endif
  if ((*env)->MonitorEnter(env, o) != JNI_OK) {
    REprintf("Rglue.warning: couldn't get monitor on the object, running unsynchronized.\n");
    return RcallMethod(par);
  }

  e = RcallMethod(par);

  if ((*env)->MonitorExit(env, o) != JNI_OK)
    REprintf("Rglue.SERIOUS PROBLEM: MonitorExit failed, subsequent calls may cause a deadlock!\n");

  return e;
}

static char *classToJNI(const char *cl) {
  if (*cl=='[') {
    char *d = strdup(cl);
    char *c = d;
    while (*c) { if (*c=='.') *c='/'; c++; }
    return d;
  }
  if (!strcmp(cl, "boolean")) return strdup("Z");
  if (!strcmp(cl, "byte"))    return strdup("B");
  if (!strcmp(cl, "int"))     return strdup("I");
  if (!strcmp(cl, "long"))    return strdup("J");
  if (!strcmp(cl, "double"))  return strdup("D");
  if (!strcmp(cl, "short"))   return strdup("S");
  if (!strcmp(cl, "float"))   return strdup("F");
  if (!strcmp(cl, "char"))    return strdup("C");
 
  /* anything else is a real class -> wrap into L..; */
  char *jc = malloc(strlen(cl)+3);
  *jc='L';
  strcpy(jc+1, cl);
  strcat(jc, ";");
  { char *c=jc; while (*c) { if (*c=='.') *c='/'; c++; } }
  return jc;
}

/* find field signature using reflection. Basically it is the same as:
   cls.getField(fnam).getType().getName()
   + class2JNI mangling */
static char *findFieldSignature(JNIEnv *env, jclass cls, const char *fnam) {
  char *detsig = 0;
  jmethodID mid = (*env)->GetMethodID(env, javaClassClass, "getField",
				      "(Ljava/lang/String;)Ljava/lang/reflect/Field;");
  if (mid) {
    jstring s = newString(env, fnam);
    if (s) {
      jobject f = (*env)->CallObjectMethod(env, cls, mid, s);
      _mp(MEM_PROF_OUT("  %08x LNEW object getField result value\n", (int) f))
      if (f) {
	jclass clField = objectClass(env, f);
	if (clField) {
	  jmethodID mid2 = (*env)->GetMethodID(env, clField, "getType",
					       "()Ljava/lang/Class;");
	  if (mid2) {
	    jobject fcl = (*env)->CallObjectMethod(env, f, mid2);
	    _mp(MEM_PROF_OUT("  %08x LNEW object getType result value\n", (int) f))
	    if (fcl) {
	      jmethodID mid3 = (*env)->GetMethodID(env, javaClassClass,
						   "getName", "()Ljava/lang/String;");
	      if (mid3) {
		jobject fcns = (*env)->CallObjectMethod(env, fcl, mid3);
		releaseObject(env, fcl);
		if (fcns) {
		  const char *fcn = (*env)->GetStringUTFChars(env, fcns, 0);
		  detsig = classToJNI(fcn);
		  _dbg(Rprintf("class '%s' -> '%s' sig\n", fcn, detsig));
		  (*env)->ReleaseStringUTFChars(env, fcns, fcn);
		  releaseObject(env, fcns);
		  /* fid = (*env)->FromReflectedField(env, f); */
		}
	      } else
		releaseObject(env, fcl);
	    }
	  }
	  releaseObject(env, clField);
	}
	releaseObject(env, f);
      }
      releaseObject(env, s);
    }
  }
  return detsig;
}

/** get value of a field of an object or class
    object (int), return signature (string), field name (string)
    arrays and objects are returned as IDs (hence not evaluated)
    class name can be in either form / or .
*/
SEXP RgetField(SEXP obj, SEXP sig, SEXP name, SEXP trueclass) {
  jobject o = 0;
  SEXP e;
  const char *retsig, *fnam;
  char *clnam = 0, *detsig = 0;
  jfieldID fid;
  jclass cls;
  int tc = asInteger(trueclass);
  int ads = 0;
  JNIEnv *env=getJNIEnv();

  if (obj == R_NilValue) return R_NilValue;
  if (inherits(obj, "jobjRef") || inherits(obj, "jarrayRef"))
    obj = GET_SLOT(obj, install("jobj"));
  if (TYPEOF(obj)==EXTPTRSXP) {
    jverify(obj);
    o=(jobject)EXTPTR_PTR(obj);
  } else if (TYPEOF(obj)==STRSXP && LENGTH(obj)==1)
    clnam = strdup(CHAR(STRING_ELT(obj, 0)));
  else
    error("invalid object parameter");
  if (!o && !clnam)
    error("cannot access a field of a NULL object");
#ifdef RJ_DEBUG
  if (o) {
    rjprintf("RgetField.object: "); printObject(env, o);
  } else {
    rjprintf("RgetFiled.class: %s\n", clnam);
  }
#endif
  if (o)
    cls = objectClass(env, o);
  else {
    char *c = clnam;
    while(*c) { if (*c=='/') *c='.'; c++; }
    cls = findClass(env, clnam);
    free(clnam);
    if (!cls) {
      error("cannot find class %s", CHAR(STRING_ELT(obj, 0)));
    }
  }
  if (!cls)
    error("cannot determine object class");
#ifdef RJ_DEBUG
  rjprintf("RgetField.class: "); printObject(env, cls);
#endif
  if (TYPEOF(name)!=STRSXP || LENGTH(name)!=1) {
    releaseObject(env, cls);
    error("invalid field name");
  }
  fnam = CHAR(STRING_ELT(name,0));
  if (sig == R_NilValue) {
    retsig = detsig = findFieldSignature(env, cls, fnam);
    if (!retsig) {
      releaseObject(env, cls);
      error("unable to detect signature for field '%s'", fnam);
    }
  } else {
    if (TYPEOF(sig)!=STRSXP || LENGTH(sig)!=1) {
      releaseObject(env, cls);
      error("invalid signature parameter");
    }
    retsig = CHAR(STRING_ELT(sig,0));
  }
  _dbg(rjprintf("field %s signature is %s\n",fnam,retsig));
  
  if (o) { /* first try non-static fields */
    fid = (*env)->GetFieldID(env, cls, fnam, retsig);
    checkExceptionsX(env, 1);
    if (!fid) { /* if that fails, try static ones */
      o = 0;
      fid = (*env)->GetStaticFieldID(env, cls, fnam, retsig);
    }
  } else /* no choice if the object was a string */
    fid = (*env)->GetStaticFieldID(env, cls, fnam, retsig);

  if (!fid) {
    checkExceptionsX(env, 1);
    releaseObject(env, cls);
    if (detsig) free(detsig);
    error("RgetField: field %s not found", fnam);
  }
  switch (*retsig) {
  case 'I': {
    int r=o?
      (*env)->GetIntField(env, o, fid):
      (*env)->GetStaticIntField(env, cls, fid);
    e = allocVector(INTSXP, 1);
    INTEGER(e)[0] = r;
    releaseObject(env, cls);
    if (detsig) free(detsig);
    return e;
  }
  case 'S': {
    jshort r=o?
      (*env)->GetShortField(env, o, fid):
      (*env)->GetStaticShortField(env, cls, fid);
    e = allocVector(INTSXP, 1);
    INTEGER(e)[0] = r;
    releaseObject(env, cls);
    if (detsig) free(detsig);
    return e;
  }
  case 'C': {
    int r=(int) o?
      (*env)->GetCharField(env, o, fid):
      (*env)->GetStaticCharField(env, cls, fid);
    e = allocVector(INTSXP, 1);
    INTEGER(e)[0] = r;
    releaseObject(env, cls);
    if (detsig) free(detsig);
    return e;
  }
  case 'B': {
    int r=(int) o?
      (*env)->GetByteField(env, o, fid):
      (*env)->GetStaticByteField(env, cls, fid);
    e = allocVector(INTSXP, 1);
    INTEGER(e)[0] = r;
    releaseObject(env, cls);
    if (detsig) free(detsig);
    return e;
  }
  case 'J': {
    jlong r=o?
      (*env)->GetLongField(env, o, fid):
      (*env)->GetStaticLongField(env, cls, fid);
    e = allocVector(REALSXP, 1);
    REAL(e)[0] = (double)r;
    releaseObject(env, cls);
    if (detsig) free(detsig);
    return e;
  }
  case 'Z': {
    jboolean r=o?
      (*env)->GetBooleanField(env, o, fid):
      (*env)->GetStaticBooleanField(env, cls, fid);
    e = allocVector(LGLSXP, 1);
    LOGICAL(e)[0] = r?1:0;
    releaseObject(env, cls);
    if (detsig) free(detsig);
    return e;
  }
  case 'D': {
    double r=o?
      (*env)->GetDoubleField(env, o, fid):
      (*env)->GetStaticDoubleField(env, cls, fid);
    e = allocVector(REALSXP, 1);
    REAL(e)[0] = r;
    releaseObject(env, cls);
    if (detsig) free(detsig);
    return e;
  }
  case 'F': {
    double r = (double) (o?
      (*env)->GetFloatField(env, o, fid):
      (*env)->GetStaticFloatField(env, cls, fid));
    e = allocVector(REALSXP, 1);
    REAL(e)[0] = r;
    releaseObject(env, cls);
    if (detsig) free(detsig);
    return e;
  }
  case 'L':
  case '[': {
    SEXP rv;
    jobject r = o?
      (*env)->GetObjectField(env, o, fid):
      (*env)->GetStaticObjectField(env, cls, fid);
    _mp(MEM_PROF_OUT("  %08x LNEW field value\n", (int) r))
    releaseObject(env, cls);
    if (tc) {
      if (detsig) free(detsig);
      return new_jobjRef(env, r, 0);
    }
    if (*retsig=='L') { /* need to fix the class name */      
      char *d = strdup(retsig), *c = d;
      while (*c) { if (*c==';') { *c=0; break; }; c++; }
      rv = new_jobjRef(env, r, d+1);
      free(d);
    } else
      rv = new_jobjRef(env, r, retsig);
    if (detsig) free(detsig);
    return rv;
  }
  } /* switch */
  releaseObject(env, cls);
  if (detsig) {
    free(detsig);
    error("unknown field signature");
  }
  error("unknown field signature '%s'", retsig);
  return R_NilValue;
}

SEXP RsetField(SEXP ref, SEXP name, SEXP value) {
  jobject o = 0, otr;
  SEXP obj = ref;
  const char *fnam;
  char sig[64];
  char *clnam = 0;
  jfieldID fid;
  jclass cls;
  jvalue jval;
  JNIEnv *env=getJNIEnv();

  if (TYPEOF(name)==STRSXP && LENGTH(name)==1)
    fnam = CHAR(STRING_ELT(name, 0));
  else
    error("invalid field name");
  if (obj == R_NilValue) error("cannot set a field of a NULL object");
  if (inherits(obj, "jobjRef") || inherits(obj, "jarrayRef"))
    obj = GET_SLOT(obj, install("jobj"));
  if (TYPEOF(obj)==EXTPTRSXP) {
    jverify(obj);
    o=(jobject)EXTPTR_PTR(obj);
  } else if (TYPEOF(obj)==STRSXP && LENGTH(obj)==1)
    clnam = strdup(CHAR(STRING_ELT(obj, 0)));
  else
    error("invalid object parameter");
  if (!o && !clnam)
    error("cannot set a field of a NULL object");
#ifdef RJ_DEBUG
  if (o) {
    rjprintf("RsetField.object: "); printObject(env, o);
  } else {
    rjprintf("RsetField.class: %s\n", clnam);
  }
#endif
  if (o)
    cls = objectClass(env, o);
  else {
    char *c = clnam;
    while(*c) { if (*c=='/') *c='.'; c++; }
    cls = findClass(env, clnam);
    if (!cls) {
      error("cannot find class %s", CHAR(STRING_ELT(obj, 0)));
    }
  }
  if (!cls)
    error("cannot determine object class");
#ifdef RJ_DEBUG
  rjprintf("RsetField.class: "); printObject(env, cls);
#endif
  *sig = 0;
  jval = R1par2jvalue(env, value, sig, &otr);
  
  if (o) {
    fid = (*env)->GetFieldID(env, cls, fnam, sig);
    if (!fid) {
      checkExceptionsX(env, 1);
      o = 0;
      fid = (*env)->GetStaticFieldID(env, cls, fnam, sig);
    }
  } else
    (*env)->GetStaticFieldID(env, cls, fnam, sig);
  if (!fid) {
    checkExceptionsX(env, 1);
    releaseObject(env, cls);
    if (otr) releaseObject(env, otr);
    error("cannot find field %s with signature %s", fnam, sig);
  }
  switch(*sig) {
  case 'Z':
    o?(*env)->SetBooleanField(env, o, fid, jval.z):
      (*env)->SetStaticBooleanField(env, cls, fid, jval.z);
    break;
  case 'C':
    o?(*env)->SetCharField(env, o, fid, jval.c):
      (*env)->SetStaticCharField(env, cls, fid, jval.c);
    break;
  case 'B':
    o?(*env)->SetByteField(env, o, fid, jval.b):
      (*env)->SetStaticByteField(env, cls, fid, jval.b);
    break;
  case 'I':
    o?(*env)->SetIntField(env, o, fid, jval.i):
      (*env)->SetStaticIntField(env, cls, fid, jval.i);
    break;
  case 'D':
    o?(*env)->SetDoubleField(env, o, fid, jval.d):
      (*env)->SetStaticDoubleField(env, cls, fid, jval.d);
    break;
  case 'F':
    o?(*env)->SetFloatField(env, o, fid, jval.f):
      (*env)->SetStaticFloatField(env, cls, fid, jval.f);
    break;
  case 'J':
    o?(*env)->SetLongField(env, o, fid, jval.j):
      (*env)->SetStaticLongField(env, cls, fid, jval.j);
    break;
  case 'S':
    o?(*env)->SetShortField(env, o, fid, jval.s):
      (*env)->SetStaticShortField(env, cls, fid, jval.s);
    break;
  case '[':
  case 'L':
    o?(*env)->SetObjectField(env, o, fid, jval.l):
      (*env)->SetStaticObjectField(env, cls, fid, jval.l);
    break;
  default:
    releaseObject(env, cls);
    if (otr) releaseObject(env, otr);
    error("unknown field sighanture %s", sig);
  }
  releaseObject(env, cls);
  if (otr) releaseObject(env, otr);
  return ref;
}

/** create new object.
    fully-qualified class in JNI notation (string) [, constructor parameters] */
SEXP RcreateObject(SEXP par) {
  SEXP p=par;
  SEXP e;
  int silent=0;
  const char *class;
  char sig[256];
  jvalue jpar[maxJavaPars];
  jobject tmpo[maxJavaPars+1];
  jobject o;
  JNIEnv *env=getJNIEnv();

  if (TYPEOF(p)!=LISTSXP) {
    _dbg(rjprintf("Parameter list expected but got type %d.\n",TYPEOF(p)));
    error_return("RcreateObject: invalid parameter");
  }

  p=CDR(p); /* skip first parameter which is the function name */
  e=CAR(p); /* second is the class name */
  if (TYPEOF(e)!=STRSXP || LENGTH(e)!=1)
    error("RcreateObject: invalid class name");
  class = CHAR(STRING_ELT(e,0));
  _dbg(rjprintf("RcreateObject: new object of class %s\n",class));
  strcpy(sig,"(");
  p=CDR(p);
  Rpar2jvalue(env, p, jpar, sig, 32, 256, tmpo);
  strcat(sig,")V");
  _dbg(rjprintf(" constructor signature is %s\n",sig));

  /* look for named arguments */
  while (TYPEOF(p)==LISTSXP) {
    if (TAG(p) && isSymbol(TAG(p))) {
      if (TAG(p)==install("silent") && isLogical(CAR(p)) && LENGTH(CAR(p))==1)
	silent=LOGICAL(CAR(p))[0];
    }
    p=CDR(p);
  }

BEGIN_RJAVA_CALL
  o = createObject(env, class, sig, jpar, silent);
END_RJAVA_CALL
  Rfreejpars(env, tmpo);
  if (!o) return R_NilValue;

#ifdef RJ_DEBUG
  {
    jstring s=callToString(env, o);
    const char *c="???";
    if (s) c=(*env)->GetStringUTFChars(env, s, 0);
    rjprintf(" new Java object [%s] reference %lx (local)\n", c, (long)o);
    if (s) {
      (*env)->ReleaseStringUTFChars(env, s, c);
      releaseObject(env, s);
    }
  }
#endif
  
  return j2SEXP(env, o, 1);
}

/** returns the name of an object's class (in the form of R string) */
static SEXP getObjectClassName(JNIEnv *env, jobject o) {
  jclass cls;
  jmethodID mid;
  jobject r;
  char cn[128];
  if (!o) return mkString("java/jang/Object");
  cls = objectClass(env, o);
  if (!cls) return mkString("java/jang/Object");
  mid = (*env)->GetMethodID(env, javaClassClass, "getName", "()Ljava/lang/String;");
  if (!mid) {
    releaseObject(env, cls);
    error("unable to get class name");
  }
  r = (*env)->CallObjectMethod(env, cls, mid);
  _mp(MEM_PROF_OUT("  %08x LNEW object getName result\n", (int) r))
  if (!r) {
    releaseObject(env, cls);
    releaseObject(env, r);
    error("unable to get class name");
  }
  cn[127]=0; *cn=0;
  {
    int sl = (*env)->GetStringLength(env, r);
    if (sl>127) {
      releaseObject(env, cls);
      releaseObject(env, r);
      error("class name is too long");
    }
    if (sl) (*env)->GetStringUTFRegion(env, r, 0, sl, cn);
  }
  { char *c=cn; while(*c) { if (*c=='.') *c='/'; c++; } }
  releaseObject(env, cls);
  releaseObject(env, r);
  return mkString(cn);
}

/** creates a new jobjREf object. If klass is NULL then the class is determined from the object (if also o=NULL then the class is set to java/lang/Object) */
static SEXP new_jobjRef(JNIEnv *env, jobject o, const char *klass) {
  SEXP oo = NEW_OBJECT(MAKE_CLASS("jobjRef"));
  if (!inherits(oo, "jobjRef"))
    error("unable to create jobjRef object");
  PROTECT(oo);
  SET_SLOT(oo, install("jclass"),
	   klass?mkString(klass):getObjectClassName(env, o));
  SET_SLOT(oo, install("jobj"), j2SEXP(env, o, 1));
  UNPROTECT(1);
  return oo;
}

static SEXP new_jarrayRef(JNIEnv *env, jobject a, const char *sig) {
  /* it is too tedious to try to do this in C, so we use 'new' R function instead */
  /* SEXP oo = eval(LCONS(install("new"),LCONS(mkString("jarrayRef"),R_NilValue)), R_GlobalEnv); */
  SEXP oo = NEW_OBJECT(MAKE_CLASS("jarrayRef"));
  /* .. and set the slots in C .. */
  if (!inherits(oo, "jarrayRef"))
    error("unable to create an array");
  PROTECT(oo);
  SET_SLOT(oo, install("jobj"), j2SEXP(env, a, 1));
  SET_SLOT(oo, install("jclass"), mkString(sig));
  SET_SLOT(oo, install("jsig"), mkString(sig));
  UNPROTECT(1);
  return oo;
}

SEXP RcreateArray(SEXP ar, SEXP cl) {
  JNIEnv *env=getJNIEnv();
  
  if (ar==R_NilValue) return R_NilValue;
  switch(TYPEOF(ar)) {
  case INTSXP:
    {
      if (inherits(ar, "jbyte")) {
	jbyteArray a = newByteArrayI(env, INTEGER(ar), LENGTH(ar));
	if (!a) error("unable to create a byte array");
	return new_jarrayRef(env, a, "[B");
      } else if (inherits(ar, "jchar")) {
	jcharArray a = newCharArrayI(env, INTEGER(ar), LENGTH(ar));
	if (!a) error("unable to create a char array");
	return new_jarrayRef(env, a, "[C");
      } else {
	jintArray a = newIntArray(env, INTEGER(ar), LENGTH(ar));
	if (!a) error("unable to create an integer array");
	return new_jarrayRef(env, a, "[I");
      }
    }
  case REALSXP:
    {
      if (inherits(ar, "jfloat")) {
	jfloatArray a = newFloatArrayD(env, REAL(ar), LENGTH(ar));
	if (!a) error("unable to create a float array");
	return new_jarrayRef(env, a, "[F");
      } else if (inherits(ar, "jlong")){
	jlongArray a = newLongArrayD(env, REAL(ar), LENGTH(ar));
	if (!a) error("unable to create a long array");
	return new_jarrayRef(env, a, "[J");
      } else {
	jdoubleArray a = newDoubleArray(env, REAL(ar), LENGTH(ar));
	if (!a) error("unable to create double array");
	return new_jarrayRef(env, a, "[D");
      }
    }
  case STRSXP:
    {
      jobjectArray a = (*env)->NewObjectArray(env, LENGTH(ar), javaStringClass, 0);
      int i = 0;
      if (!a) error("unable to create a string array");
      while (i<LENGTH(ar)) {
	jobject so = newString(env, CHAR(STRING_ELT(ar, i)));
	(*env)->SetObjectArrayElement(env, a, i, so);
	releaseObject(env, so);
	i++;
      }
      return new_jarrayRef(env, a, "[Ljava/lang/String;");
    }
  case LGLSXP:
    {
      /* ASSUMPTION: LOGICAL()=int* */
      jbooleanArray a = newBooleanArrayI(env, LOGICAL(ar), LENGTH(ar));
      if (!a) error("unable to create a boolean array");
      return new_jarrayRef(env, a, "[Z");
    }
  case VECSXP:
    {
      int i=0;
      jclass ac = javaObjectClass;
      const char *sigName = 0;
      char buf[256];
      
      while (i<LENGTH(ar)) {
	SEXP e = VECTOR_ELT(ar, i);
	if (e != R_NilValue &&
	    !inherits(e, "jobjRef") &&
	    !inherits(e, "jarrayRef"))
	  error("Cannot create a Java array from a list that contains anything other than Java object references.");
	i++;
      }
      /* optional class name for the objects contained in the array */
      if (TYPEOF(cl)==STRSXP && LENGTH(cl)>0) {
	const char *cname = CHAR(STRING_ELT(cl, 0));
	if (cname) {
	  ac = findClass(env, cname);
	  if (!ac)
	    error("Cannot find class %s.", cname);
	  if (strlen(cname)<253) {
	    /* it's valid to have [* for class name (for mmulti-dim
	       arrays), but then we cannot add [L..; */
	    if (*cname == '[') {
	      /* we have to add [ prefix to the signature */
	      buf[0] = '[';
	      strcpy(buf+1, cname);
	    } else {
	      buf[0] = '['; buf[1] = 'L'; 
	      strcpy(buf+2, cname);
	      strcat(buf,";");
	    }
	    sigName = buf;
	  }
	}
      } /* if contents class specified */
      {
	jobjectArray a = (*env)->NewObjectArray(env, LENGTH(ar), ac, 0);
	_mp(MEM_PROF_OUT("  %08x LNEW object[%d]\n", (int)a, LENGTH(ar)))
	if (ac != javaObjectClass) releaseObject(env, ac);
	i=0;
	if (!a) error("Cannot create array of objects.");
	while (i < LENGTH(ar)) {
	  SEXP e = VECTOR_ELT(ar, i);
	  jobject o = 0;
	  if (e != R_NilValue) {
	    SEXP sref = GET_SLOT(e, install("jobj"));
	    if (sref && TYPEOF(sref) == EXTPTRSXP) {
	      jverify(sref);
	      o = (jobject)EXTPTR_PTR(sref);
	    }
	  }	  
	  (*env)->SetObjectArrayElement(env, a, i, o);
	  i++;
	}
	return new_jarrayRef(env, a, sigName?sigName:"[Ljava/lang/Object;");
      }
    }
  case RAWSXP:
    {
      jbyteArray a = newByteArray(env, RAW(ar), LENGTH(ar));
      if (!a) error("unable to create a byte array");
      return new_jarrayRef(env, a, "[B");
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
  if (TYPEOF(e)==EXTPTRSXP) {
    jverify(e);
    o = (jobject)EXTPTR_PTR(e);
  } else
    error_return("RfreeObject: invalid object parameter");
  _dbg(rjprintf("RfreeObject: release reference %lx\n", (long)o));
BEGIN_RJAVA_CALL
  releaseGlobal(env, o);
END_RJAVA_CALL
  return R_NilValue;
}

/** create a NULL external reference */
SEXP RgetNullReference() {
  return R_MakeExternalPtr(0, R_NilValue, R_NilValue);
}

/** check whether there is an exception pending and
    return the exception if any (NULL otherwise) */
SEXP RpollException() {
  JNIEnv *env=getJNIEnv();
  jthrowable t;
BEGIN_RJAVA_CALL
  t=(*env)->ExceptionOccurred(env);
END_RJAVA_CALL
  _mp(MEM_PROF_OUT("  %08x LNEW RpollException throwable\n", (int)t))
  return t?j2SEXP(env, t, 1):R_NilValue;
}

/** clear any pending exceptions */
void RclearException() {
  JNIEnv *env=getJNIEnv();
BEGIN_RJAVA_CALL
  (*env)->ExceptionClear(env);  
END_RJAVA_CALL
}

SEXP RthrowException(SEXP ex) {
  JNIEnv *env=getJNIEnv();
  jthrowable t=0;
  SEXP exr;
  int tr=0;
  SEXP res;

  if (!inherits(ex, "jobjRef"))
    error("Invalid throwable object.");
  
  exr=GET_SLOT(ex, install("jobj"));
  if (exr && TYPEOF(exr)==EXTPTRSXP) {
    jverify(exr);
    t=(jthrowable)EXTPTR_PTR(exr);
  }
  if (!t)
    error("Throwable must be non-null.");
  
BEGIN_RJAVA_CALL
  tr = (*env)->Throw(env, t);
END_RJAVA_CALL
  res = allocVector(INTSXP, 1);
  INTEGER(res)[0]=tr;
  return res;
}

/** TRUE if cl1 x; cl2 y = (cl2) x ... is valid */
SEXP RisAssignableFrom(SEXP cl1, SEXP cl2) {
  SEXP r;
  JNIEnv *env=getJNIEnv();

  if (TYPEOF(cl1)!=EXTPTRSXP || TYPEOF(cl2)!=EXTPTRSXP)
    error("invalid type");
  if (!env)
    error("VM not initialized");
  jverify(cl1);
  jverify(cl2);
  r=allocVector(LGLSXP,1);
  LOGICAL(r)[0]=((*env)->IsAssignableFrom(env,
					  (jclass)EXTPTR_PTR(cl1),
					  (jclass)EXTPTR_PTR(cl2)));
  return r;
}

SEXP RJava_checkJVM() {
  SEXP r = allocVector(LGLSXP, 1);
  LOGICAL(r)[0] = 0;
  if (!jvm || !getJNIEnv()) return r;
  LOGICAL(r)[0] = 1;
  return r;
}

extern int rJava_initialized; /* in callJNI.c */

SEXP RJava_needs_init() {
  SEXP r = allocVector(LGLSXP, 1);
  LOGICAL(r)[0] = rJava_initialized?0:1;
  return r;
}

SEXP RJava_set_class_loader(SEXP ldr) {
  JNIEnv *env=getJNIEnv();
  if (TYPEOF(ldr) != EXTPTRSXP)
    error("invalid type");
  if (!env)
    error("VM not initialized");
  
  jverify(ldr);
  initClassLoader(env, (jobject)EXTPTR_PTR(ldr));
  return R_NilValue;
}

SEXP RJava_primary_class_loader() {
  JNIEnv *env=getJNIEnv();
  jclass cl = (*env)->FindClass(env, "RJavaClassLoader");
  Rprintf("RJava_primary_class_loader, cl = %x\n", (int) cl);
  if (cl) {
    jmethodID mid = (*env)->GetStaticMethodID(env, cl, "getPrimaryLoader", "()LRJavaClassLoader;");
    Rprintf(" - mid = %d\n", (int) mid);
    if (mid) {
      jobject o = (*env)->CallStaticObjectMethod(env, cl, mid);
      Rprintf(" - call result = %x\n", (int) o);
      if (o) {
	return j2SEXP(env, o, 1);
      }
    }
  }
  checkExceptionsX(env, 1);

#ifdef NEW123
  jclass cl = (*env)->FindClass(env, "JRIBootstrap");
  Rprintf("RJava_primary_class_loader, cl = %x\n", (int) cl);
  if (cl) {
    jmethodID mid = (*env)->GetStaticMethodID(env, cl, "getBootRJavaLoader", "()Ljava/lang/Object;");
    Rprintf(" - mid = %d\n", (int) mid);
    if (mid) {
      jobject o = (*env)->CallStaticObjectMethod(env, cl, mid);
      Rprintf(" - call result = %x\n", (int) o);
      if (o) {
	return j2SEXP(env, o, 1);
      }
    }
  }
  checkExceptionsX(env, 1);
#endif
  return R_NilValue; 
}

SEXP RJava_new_class_loader(SEXP p1, SEXP p2) {
  JNIEnv *env=getJNIEnv();
  
  const char *c1 = CHAR(STRING_ELT(p1, 0));
  const char *c2 = CHAR(STRING_ELT(p2, 0));
  jstring s1 = newString(env, c1);
  jstring s2 = newString(env, c2);

  jclass cl = (*env)->FindClass(env, "RJavaClassLoader");
  Rprintf("find rJavaClassLoader: %x\n", (int) cl);
  jmethodID mid = (*env)->GetMethodID(env, cl, "<init>", "(Ljava/lang/String;Ljava/lang/String;)V");
  Rprintf("constructor mid: %x\n", mid);
  jobject o = (*env)->NewObject(env, cl, mid, s1, s2);
  Rprintf("new object: %x\n", o);
  o = makeGlobal(env, o);
  Rprintf("calling initClassLoader\n");
  initClassLoader(env, o);
  releaseObject(env, s1);
  releaseObject(env, s2);
  releaseObject(env, cl);
  return R_NilValue;
}

SEXP RJava_set_memprof(SEXP fn) {
#ifdef MEMPROF
  const char *cFn = CHAR(STRING_ELT(fn, 0));
  int env = 0; /* we're just faking it so we can call MEM_PROF_OUT */
  
  if (memprof_f) fclose(memprof_f);
  if (cFn && !cFn[0]) {
    memprof_f = 0; return R_NilValue;
  }
  if (!cFn || (cFn[0]=='-' && !cFn[1]))
    memprof_f = stdout;
  else
    memprof_f = fopen(cFn, "a");
  if (!memprof_f) error("Cannot create memory profile file.");
  MEM_PROF_OUT("  00000000 REST -- profiling file set --\n");
  return R_NilValue;
#else
  error("Memory profiling support was not enabled in rJava.");
#endif
  return R_NilValue;
}
