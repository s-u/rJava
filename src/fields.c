/* R-callable functions to get/set fields
 *
 * rJava R/Java interface  (C)Copyright 2003-2007 Simon Urbanek
 * (see rJava project root for licensing details)
 */

#include "rJava.h"
#include <Rdefines.h>

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

static SEXP R_Sym_jobj = 0;

/* get an R object and extract Java class and Java object from it.
   if it is a string of class reference then jobj will
   be NULL and only the class is provided.
   is_local is set to1 if the returned cls is a local reference
   that needs to be released, 0 otherwise (typically if it is NULL
   or comes from jclassName object)
   FIXME: This is fairly generic - should we move it to tools?
*/
static jclass inputToClass(JNIEnv *env, SEXP obj, jobject *jobj, int *is_local) {
  jclass cls = 0;
  jobject o = 0;

  if (is_local)
    *is_local = 0;

  if (!R_Sym_jobj)
    R_Sym_jobj = Rf_install("jobj");

  /* jclassName is the result of J("class.name") and has the class object in jobj slot */
  if (inherits(obj, "jclassName")) {
    obj = GET_SLOT(obj, R_Sym_jobj);
    jverify(obj); /* twice wrapped: className has @jobj slot which in turn contains jobjRef to the class */
    obj = GET_SLOT(obj, R_Sym_jobj);
    jverify(obj);
    cls = (jclass)EXTPTR_PTR(obj);
#ifdef RJ_DEBUG
    if (cls) {
      rjprintf("inputToClass, class: "); printObject(env, cls);
    }
#endif
  } else {
    char *clnam = 0;

    if (IS_JOBJREF(obj)) /* any of the jobjRef derivates */
      obj = GET_SLOT(obj, install("jobj"));
    if (TYPEOF(obj) == EXTPTRSXP) {
      jverify(obj);
      o = (jobject)EXTPTR_PTR(obj);
    } else if (TYPEOF(obj) == STRSXP && LENGTH(obj) == 1)
      clnam = strdup(CHAR(STRING_ELT(obj, 0)));
    else
      error("invalid object parameter");
    if (!o && !clnam)
      error("cannot access a NULL object");
#ifdef RJ_DEBUG
    if (o) {
      rjprintf("inputToClass, object: "); printObject(env, o);
    } else {
      rjprintf("inputToClass, class: %s\n", clnam);
    }
#endif
    if (o)
      cls = objectClass(env, o);
    else { /* this should be rare since is doesn't provide a way to specify the class loader */
      cls = findClass(env, clnam, oClassLoader);
      free(clnam);
      if (!cls) {
	error("cannot find class %s", CHAR(STRING_ELT(obj, 0)));
      }
    }
    if (cls && is_local)
      *is_local = 1;
  }
  if (jobj)
    *jobj = o;
  return cls;
}


/* find field signature using reflection. Basically it is the same as:
   cls.getField(fnam).getType().getName()
   + class2JNI mangling */
/* TODO; use the mid_RJavaTools_getFieldTypeName method ID instead */
static char *findFieldSignature(JNIEnv *env, jclass cls, const char *fnam) {
  char *detsig = 0;
  jstring s = newString(env, fnam);
  if (s) {
    jobject f = (*env)->CallObjectMethod(env, cls, mid_getField, s);
    _mp(MEM_PROF_OUT("  %08x LNEW object getField result value\n", (int) f))
    if (f) {
      jobject fcl = (*env)->CallObjectMethod(env, f, mid_getType);
      _mp(MEM_PROF_OUT("  %08x LNEW object getType result value\n", (int) f))
      if (fcl) {
	jobject fcns = (*env)->CallObjectMethod(env, fcl, mid_getName);
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
      releaseObject(env, f);
    }
    releaseObject(env, s);
  }
  return detsig;
}

/** get value of a field of an object or class
    object (int), return signature (string), field name (string)
    arrays and objects are returned as IDs (hence not evaluated)
    class name can be in either form / or .
*/
REPC SEXP RgetField(SEXP obj, SEXP sig, SEXP name, SEXP trueclass) {
  jobject o = 0;
  SEXP e;
  const char *retsig, *fnam;
  char *detsig = 0;
  jfieldID fid;
  jclass cls;
  int tc = asInteger(trueclass), cls_local = 0;
  JNIEnv *env=getJNIEnv();

  if (obj == R_NilValue) return R_NilValue;
  cls = inputToClass(env, obj, &o, &cls_local);
  if (!cls)
    error("cannot determine object class");
#ifdef RJ_DEBUG
  rjprintf("RgetField.class: "); printObject(env, cls);
#endif
  if (TYPEOF(name)!=STRSXP || LENGTH(name)!=1) {
    if (cls_local) releaseObject(env, cls);
    error("invalid field name");
  }
  fnam = CHAR(STRING_ELT(name,0));
  if (sig == R_NilValue) {
    retsig = detsig = findFieldSignature(env, cls, fnam);
    if (!retsig) {
      if (cls_local) releaseObject(env, cls);
      error("unable to detect signature for field '%s'", fnam);
    }
  } else {
    if (TYPEOF(sig)!=STRSXP || LENGTH(sig)!=1) {
      if (cls_local) releaseObject(env, cls);
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
    if (cls_local) releaseObject(env, cls);
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
    if (cls_local) releaseObject(env, cls);
    if (detsig) free(detsig);
    return e;
  }
  case 'S': {
    jshort r=o?
      (*env)->GetShortField(env, o, fid):
      (*env)->GetStaticShortField(env, cls, fid);
    e = allocVector(INTSXP, 1);
    INTEGER(e)[0] = r;
    if (cls_local) releaseObject(env, cls);
    if (detsig) free(detsig);
    return e;
  }
  case 'C': {
    int r=(int) (o?
		 (*env)->GetCharField(env, o, fid):
		 (*env)->GetStaticCharField(env, cls, fid));
    e = allocVector(INTSXP, 1);
    INTEGER(e)[0] = r;
    if (cls_local) releaseObject(env, cls);
    if (detsig) free(detsig);
    return e;
  }
  case 'B': {
    int r=(int) (o?
		 (*env)->GetByteField(env, o, fid):
		 (*env)->GetStaticByteField(env, cls, fid));
    e = allocVector(INTSXP, 1);
    INTEGER(e)[0] = r;
    if (cls_local) releaseObject(env, cls);
    if (detsig) free(detsig);
    return e;
  }
  case 'J': {
    jlong r=o?
      (*env)->GetLongField(env, o, fid):
      (*env)->GetStaticLongField(env, cls, fid);
    e = allocVector(REALSXP, 1);
    REAL(e)[0] = (double)r;
    if (cls_local) releaseObject(env, cls);
    if (detsig) free(detsig);
    return e;
  }
  case 'Z': {
    jboolean r=o?
      (*env)->GetBooleanField(env, o, fid):
      (*env)->GetStaticBooleanField(env, cls, fid);
    e = allocVector(LGLSXP, 1);
    LOGICAL(e)[0] = r?1:0;
    if (cls_local) releaseObject(env, cls);
    if (detsig) free(detsig);
    return e;
  }
  case 'D': {
    double r=o?
      (*env)->GetDoubleField(env, o, fid):
      (*env)->GetStaticDoubleField(env, cls, fid);
    e = allocVector(REALSXP, 1);
    REAL(e)[0] = r;
    if (cls_local) releaseObject(env, cls);
    if (detsig) free(detsig);
    return e;
  }
  case 'F': {
    double r = (double) (o?
      (*env)->GetFloatField(env, o, fid):
      (*env)->GetStaticFloatField(env, cls, fid));
    e = allocVector(REALSXP, 1);
    REAL(e)[0] = r;
    if (cls_local) releaseObject(env, cls);
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
    if (cls_local) releaseObject(env, cls);
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
  if (cls_local) releaseObject(env, cls);
  if (detsig) {
    free(detsig);
    error("unknown field signature");
  }
  error("unknown field signature '%s'", retsig);
  return R_NilValue;
}

REPC SEXP RsetField(SEXP ref, SEXP name, SEXP value) {
  jobject o = 0, otr;
  SEXP obj = ref;
  const char *fnam;
  sig_buffer_t sig;
  jfieldID fid;
  jclass cls;
  jvalue jval;
  int cls_local = 0;
  JNIEnv *env=getJNIEnv();

  if (TYPEOF(name)!=STRSXP && LENGTH(name)!=1)
    error("invalid field name");
  fnam = CHAR(STRING_ELT(name, 0));
  if (obj == R_NilValue) error("cannot set a field of a NULL object");
  cls = inputToClass(env, obj, &o, &cls_local);
  if (!cls)
    error("cannot determine object class");
#ifdef RJ_DEBUG
  rjprintf("RsetField.class: "); printObject(env, cls);
#endif
  init_sigbuf(&sig);
  jval = R1par2jvalue(env, value, &sig, &otr);

  if (o) {
    fid = (*env)->GetFieldID(env, cls, fnam, sig.sig);
    if (!fid) {
      checkExceptionsX(env, 1);
      o = 0;
      fid = (*env)->GetStaticFieldID(env, cls, fnam, sig.sig);
    }
  } else
    fid = (*env)->GetStaticFieldID(env, cls, fnam, sig.sig);
  if (!fid) {
    checkExceptionsX(env, 1);
    if (cls_local) releaseObject(env, cls);
    if (otr) releaseObject(env, otr);
    done_sigbuf(&sig);
    error("cannot find field %s with signature %s", fnam, sig.sigbuf);
  }
  switch(sig.sig[0]) {
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
    if (cls_local) releaseObject(env, cls);
    if (otr) releaseObject(env, otr);
    done_sigbuf(&sig);
    error("unknown field siganture %s", sig.sigbuf);
  }
  done_sigbuf(&sig);
  if (cls_local) releaseObject(env, cls);
  if (otr) releaseObject(env, otr);
  return ref;
}
