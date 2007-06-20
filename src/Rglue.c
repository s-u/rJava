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
jvalue R1par2jvalue(JNIEnv *env, SEXP par, char *sig, jobject *otr) {
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

/** creates a new jobjRef object. If klass is NULL then the class is determined from the object (if also o=NULL then the class is set to java/lang/Object) */
SEXP new_jobjRef(JNIEnv *env, jobject o, const char *klass) {
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
