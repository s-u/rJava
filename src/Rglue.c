#include "rJava.h"
#include <R.h>
#include <Rdefines.h>
#include <Rinternals.h>

SEXP RinitJVM(SEXP par)
{
  char *c=0;
  SEXP e=CADR(par);
  int r;
  
  if (TYPEOF(e)==STRSXP && LENGTH(e)>0)
    c=CHAR(STRING_ELT(e,0));
  r=initJVM(c);
  PROTECT(e=allocVector(INTSXP,1));
  INTEGER(e)[0]=r;
  UNPROTECT(1);
  return e;
}

/* converts parameters in SEXP list to jpar and sig.
   strcat is used on sig, hence sig mut be a valid string already */
SEXP Rpar2jvalue(SEXP par, jvalue *jpar, char *sig, int maxpar, int maxsig) {
  SEXP p=par;
  SEXP e;
  int jvpos=0;
  int i=0;

  while (p && TYPEOF(p)==LISTSXP && (e=CAR(p))) {
    printf("par %d type %d\n",i,TYPEOF(e));
    if (TYPEOF(e)==STRSXP) {
      printf(" string vector of length %d\n",LENGTH(e));
      if (LENGTH(e)==1) {
	strcat(sig,"Ljava/lang/String;");
	jpar[jvpos++].l=newString(CHAR(STRING_ELT(e,0)));
      } else {
	int j=0;
	jobjectArray sa=(*env)->NewObjectArray(env, LENGTH(e), getClass("java/lang/String"), 0);
	if (!sa) error_return("Unable to create string array.");
	while (j<LENGTH(e)) {
	  jobject s=newString(CHAR(STRING_ELT(e,j)));
	  printf (" [%d] \"%s\"\n",j,CHAR(STRING_ELT(e,j)));
	  (*env)->SetObjectArrayElement(env,sa,j,s);
	  j++;
	}
	jpar[jvpos++].l=sa;
	strcat(sig,"[Ljava/lang/String;");
      }
    } else if (TYPEOF(e)==INTSXP) {
      printf(" integer vector of length %d\n",LENGTH(e));
      if (LENGTH(e)==1) {
	strcat(sig,"I");
	jpar[jvpos++].i=(jint)(INTEGER(e)[0]);
	printf("  single int orig=%d, jarg=%d [jvpos=%d]\n",
	       (INTEGER(e)[0]),
	       jpar[jvpos-1],
	       jvpos);
      } else {
	strcat(sig,"[I");
	jpar[jvpos++].l=newIntArray(INTEGER(e),LENGTH(e));
      }
    } else if (TYPEOF(e)==REALSXP) {
      printf(" real vector of length %d\n",LENGTH(e));
      if (LENGTH(e)==1) {
	strcat(sig,"D");
	jpar[jvpos++].d=(jdouble)(REAL(e)[0]);
      } else {
	strcat(sig,"[D");
	jpar[jvpos++].l=newDoubleArray(REAL(e),LENGTH(e));
      }
    } else if (TYPEOF(e)==LGLSXP) {
      printf(" logical vector of length %d\n",LENGTH(e));
      if (LENGTH(e)==1) {
	strcat(sig,"Z");
	jpar[jvpos++].z=(jboolean)(LOGICAL(e)[0]);
      } else {
	strcat(sig,"[Z");
	jpar[jvpos++].l=newBooleanArrayI(LOGICAL(e),LENGTH(e));
      }
    } else if (TYPEOF(e)==VECSXP) {
      int j=0;
      printf(" general vector of length %d\n", LENGTH(e));
      if (inherits(e,"jobjRef")) {
	jobject o=(jobject)0;
	char *jc=0;
	SEXP n=getAttrib(e, R_NamesSymbol);
	if (TYPEOF(n)!=STRSXP) n=0;
	printf(" which is in fact a Java object reference\n");
	while (j<LENGTH(e)) {
	  SEXP ve=VECTOR_ELT(e,j);
	  printf("  element %d type %d\n",j,TYPEOF(ve));
	  if (n && j<LENGTH(n)) {
	    char *an=CHAR(STRING_ELT(n,j));
	    printf("  name: %s\n",an);
	    if (!strcmp(an,"jobj") && TYPEOF(ve)==INTSXP && LENGTH(ve)==1)
	      o=(jobject)INTEGER(ve)[0];
	    if (!strcmp(an,"jclass") && TYPEOF(ve)==STRSXP && LENGTH(ve)==1)
	      jc=CHAR(STRING_ELT(ve,0));
	  }
	  j++;
	}
	if (jc) {
	  strcat(sig,"L"); strcat(sig,jc); strcat(sig,";");
	} else
	  strcat(sig,"Ljava/lang/Object;");
	jpar[jvpos++].l=o;
      } else {
	printf(" (ignoring)\n");
      }
    }
    i++;
    p=CDR(p);
  }
  return R_NilValue;
}

/* jobjRefInt object : string */
SEXP RgetStringValue(SEXP par) {
  SEXP p,e,r;
  jstring s;
  const char *c;

  p=CDR(par); e=CAR(p); p=CDR(p);
  if (TYPEOF(e)!=INTSXP)
    error_return("RgetStringValue: invalid object parameter");
  s=(jstring)INTEGER(e)[0];
  if (!s) return R_NilValue;
  c=(*env)->GetStringUTFChars(env, s, 0);
  if (!c)
    error_return("RgetStringValue: can't retrieve string content");
  PROTECT(r=allocVector(STRSXP,1));
  SET_STRING_ELT(r, 0, mkChar(c));
  UNPROTECT(1);
  (*env)->ReleaseStringUTFChars(env, s, c);
  return r;
}

SEXP RtoString(SEXP par) {
  SEXP p,e,r;
  jstring s;
  jobject o;
  jclass cls;
  jmethodID mid;
  const char *c;

  p=CDR(par); e=CAR(p); p=CDR(p);
  if (TYPEOF(e)!=INTSXP)
    error_return("RtoString: invalid object parameter");
  o=(jobject)INTEGER(e)[0];
  if (!o) return R_NilValue;
  cls=(*env)->GetObjectClass(env,o);
  if (!cls) error_return("RtoString: can't determine class of the object");
  mid=(*env)->GetMethodID(env, cls, "toString", "()Ljava/lang/String;");
  if (!mid) error_return("RtoString: toString not found for the object");
  s=(jstring)(*env)->CallObjectMethod(env, o, mid);
  if (!s) error_return("RtoString: toString call failed");
  c=(*env)->GetStringUTFChars(env, s, 0);
  PROTECT(r=allocVector(STRSXP,1));
  SET_STRING_ELT(r, 0, mkChar(c));
  UNPROTECT(1);
  (*env)->ReleaseStringUTFChars(env, s, c);
  return r;
}

SEXP RgetIntArrayCont(SEXP par) {
  SEXP e=CAR(CDR(par));
  SEXP ar;
  jarray o;
  int l;
  jint *ap;

  if (TYPEOF(e)!=INTSXP)
    error_return("RgetIntArrayCont: invalid object parameter");
  o=(jarray)INTEGER(e)[0];
  printf(" jarray %d\n",o);
  if (!o) return R_NilValue;
  l=(int)(*env)->GetArrayLength(env, o);
  printf("convert int array of length %d\n",l);
  if (l<1) return R_NilValue;
  ap=(jint*)(*env)->GetIntArrayElements(env, o, 0);
  if (!ap)
    error_return("RgetIntArrayCont: can't fetch array contents");
  PROTECT(ar=allocVector(INTSXP,l));
  memcpy(INTEGER(ar),ap,sizeof(jint)*l);
  UNPROTECT(1);
  (*env)->ReleaseIntArrayElements(env, o, ap, 0);
  return ar;
}

SEXP RcallMethod(SEXP par) {
  SEXP p=par, e;
  char sig[256];
  jvalue jpar[32];
  jobject o;
  char *retsig, *mnam;
  jmethodID mid;
  jclass cls;

  p=CDR(p); e=CAR(p); p=CDR(p);
  if (TYPEOF(e)!=INTSXP)
    error_return("RcallMethod: invalid object parameter");
  o=(jobject)(INTEGER(e)[0]);
#ifdef RJAVA_DEBUG
  printf("object: "); printObject(o);
#endif
  cls=(*env)->GetObjectClass(env,o);
  if (!cls)
    error_return("RcallMethod: cannot determine object class");
#ifdef RJAVA_DEBUG
  printf("class: "); printObject(cls);
#endif
  e=CAR(p); p=CDR(p);
  if (TYPEOF(e)!=STRSXP || LENGTH(e)!=1)
    error_return("RcallMethod: invalid return signature parameter");
  retsig=CHAR(STRING_ELT(e,0));
  e=CAR(p); p=CDR(p);
  if (TYPEOF(e)!=STRSXP || LENGTH(e)!=1)
    error_return("RcallMethod: invalid method name");
  mnam=CHAR(STRING_ELT(e,0));
  strcpy(sig,"(");
  Rpar2jvalue(p,jpar,sig,32,256);
  strcat(sig,")");
  strcat(sig,retsig);
  printf("Method %s signature is %s\n",mnam,sig);
  mid=(*env)->GetMethodID(env,cls,mnam,sig);
  if (!mid)
    error_return("RcallMethod: method not found");
  if (*retsig=='V') {
    (*env)->CallVoidMethodA(env,o,mid,jpar);
    return R_NilValue;
  }
  if (*retsig=='I') {
    int r=(*env)->CallIntMethodA(env,o,mid,jpar);
    PROTECT(e=allocVector(INTSXP, 1));
    INTEGER(e)[0]=r;
    UNPROTECT(1);
    return e;
  }
  if (*retsig=='D') {
    double r=(*env)->CallDoubleMethodA(env,o,mid,jpar);
    PROTECT(e=allocVector(REALSXP, 1));
    REAL(e)[0]=r;
    UNPROTECT(1);
    return e;
  }
  if (*retsig=='L' || *retsig=='[') {
    jobject gr;
    jobject r=(*env)->CallObjectMethodA(env,o,mid,jpar);
    gr=r;
    if (r) {
      gr=makeGlobal(r);
      if (gr)
	releaseObject(r);
    }
    PROTECT(e=allocVector(INTSXP, 1));
    INTEGER(e)[0]=(int)gr;
    UNPROTECT(1);
    return e;
  }
  return R_NilValue;
}

SEXP RcreateObject(SEXP par) {
  SEXP p=par;
  SEXP e, ov;
  char *class;
  char sig[256];
  jvalue jpar[32];
  jobject o,go;

  if (TYPEOF(p)!=LISTSXP) {
    printf("Parameter list expected but got type %d.\n",TYPEOF(p));
    error_return("RcreateObject: invalid parameter");
  }

  p=CDR(p); /* skip first parameter which is the function name */
  e=CAR(p); /* second is the class name */
  if (TYPEOF(e)!=STRSXP || LENGTH(e)!=1)
    error_return("RcreateObject: invalid class name");
  class=CHAR(STRING_ELT(e,0));
  printf("new %s(...)\n",class);
  strcpy(sig,"(");
  p=CDR(p);
  Rpar2jvalue(p,jpar,sig,32,256);
  strcat(sig,")V");
  printf("Constructor signature is %s\n",sig);
  o=createObject(class,sig,jpar);
  go=makeGlobal(o);
  if (go)
    releaseObject(o);
  else
    go=o;
  PROTECT(ov=allocVector(INTSXP, 1));
  INTEGER(ov)[0]=(int)go;
  UNPROTECT(1);
  return ov;
}

/* jobjRefInt object : string */
SEXP RfreeObject(SEXP par) {
  SEXP p,e;
  jobject o;

  p=CDR(par); e=CAR(p); p=CDR(p);
  if (TYPEOF(e)!=INTSXP)
    error_return("RfreeObject: invalid object parameter");
  o=(jobject)INTEGER(e)[0];
  releaseGlobal(o);
  return R_NilValue;
}
