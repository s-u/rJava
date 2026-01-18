#include <R.h>
#include <Rversion.h>
#include <Rdefines.h>
#include "rJava.h"
#include <stdlib.h>
#include <string.h>

/* determine whether eenv cache should be used (has no effect if JNI_CACHE is not set) */
int use_eenv = 1;

/* cached environment. Do NOT use directly! Always use getJNIEnv()! */
JNIEnv *eenv;

static SEXP getCurrentCall(void) {
    SEXP cexp, sys_calls = PROTECT(install("sys.calls"));
    cexp = PROTECT(lang1(sys_calls));
    SEXP cl = eval(cexp, R_GetCurrentEnv());
    UNPROTECT(2);
    /* find the last call */
    if (TYPEOF(cl) != LISTSXP) return R_NilValue;
    while (cl != R_NilValue) {
	if (CDR(cl) == R_NilValue && CAR(cl) != R_NilValue)
	    return CAR(cl);
	cl = CDR(cl);
    }
    return R_NilValue; /* (LENGTH(cl) > 0) ? VECTOR_ELT(cl, 0) : R_NilValue; */
}

/** throw an exception using R condition code.
 *  @param msg - message string
 *  @param jobj - jobjRef object of the exception
 *  @param clazzes - simple name of all the classes in the inheritance tree of the exception plus "error" and "condition"
 */
HIDE void throwR(SEXP msg, SEXP jobj, SEXP clazzes) {
	SEXP cond = PROTECT(allocVector(VECSXP, 3));
	SEXP names = PROTECT(allocVector(STRSXP, 3));
	SET_VECTOR_ELT(cond, 0, msg);
	SET_VECTOR_ELT(cond, 1, getCurrentCall());
	SET_VECTOR_ELT(cond, 2, jobj);
	SET_STRING_ELT(names, 0, mkChar("message"));
	SET_STRING_ELT(names, 1, mkChar("call"));
	SET_STRING_ELT(names, 2, mkChar("jobj"));

	setAttrib(cond, R_NamesSymbol, names);
	setAttrib(cond, R_ClassSymbol, clazzes);
	UNPROTECT(1); /* names */
	eval(PROTECT(LCONS(install("stop"), PROTECT(CONS(cond, R_NilValue)))), R_GlobalEnv);
	UNPROTECT(3); /* cond + eval-pars */
}

/* check for exceptions and throw them to R level */
HIDE void ckx(JNIEnv *env) {
	SEXP xr, xobj, msg = 0, xclass = 0; /* note: we don't bother counting protections because we never return */
	jthrowable x = 0;
	if (env && !(x = (*env)->ExceptionOccurred(env))) return;
	if (!env) {
		env = getJNIEnv();
		if (!env)
			error("Unable to retrieve JVM environment.");
		ckx(env);
		return;
	}
	/* env is valid and an exception occurred */
	/* we create the jobj first, because the exception may in theory disappear after being cleared,
	   yet this can be (also in theory) risky as it uses further JNI calls ... */
	xobj = PROTECT(j2SEXP(env, x, 0));
	if (!rj_RJavaTools_Class) {
	    REprintf("ERROR: Java exception occurred during rJava bootstrap - see stderr for Java stack trace.\n");
	    (*env)->ExceptionDescribe(env);
	}
	(*env)->ExceptionClear(env);

	/* grab the list of class names (without package path) */
	SEXP clazzes = PROTECT( rj_RJavaTools_Class ? getSimpleClassNames_asSEXP( (jobject)x, (jboolean)1 ) : R_NilValue );

	/* ok, now this is a critical part that we do manually to avoid recursion */
	{
		jclass cls = (*env)->GetObjectClass(env, x);
		if (cls) {
			jstring cname;
			jmethodID mid = (*env)->GetMethodID(env, cls, "toString", "()Ljava/lang/String;");
			if (mid) {
				jstring s = (jstring)(*env)->CallObjectMethod(env, x, mid);
				if (s) {
					const char *c = (*env)->GetStringUTFChars(env, s, 0);
					if (c) {
						msg = PROTECT(mkString(c));
						(*env)->ReleaseStringUTFChars(env, s, c);
					}
				}
			}
			/* beside toString() we also need to call getName() on cls to get the subclass */
			cname = (jstring) (*env)->CallObjectMethod(env, cls, mid_getName);
			if (cname) {
				const char *c = (*env)->GetStringUTFChars(env, cname, 0);
				if (c) {
					/* convert full class name to JNI notation */
					char *cn = strdup(c), *d = cn;
					while (*d) { if (*d == '.') *d = '/'; d++; }
					xclass = PROTECT(mkString(cn));
					free(cn);
					(*env)->ReleaseStringUTFChars(env, cname, c);
				}
				(*env)->DeleteLocalRef(env, cname);
			}
			if ((*env)->ExceptionOccurred(env))
				(*env)->ExceptionClear(env);
			(*env)->DeleteLocalRef(env, cls);
		} else (*env)->ExceptionClear(env);
		if (!msg)
			msg = PROTECT(mkString("Java Exception <no description because toString() failed>"));
	}
	/* delete the local reference to the exception (jobjRef has a global copy) */
	(*env)->DeleteLocalRef(env, x);

	/* construct the jobjRef */
	xr = PROTECT(NEW_OBJECT(PROTECT(MAKE_CLASS("jobjRef"))));
	if (!xclass) xclass = PROTECT(mkString("java/lang/Throwable"));
	if (inherits(xr, "jobjRef")) {
		SET_SLOT(xr, install("jclass"), xclass);
		SET_SLOT(xr, install("jobj"), xobj);
	}

	/* and off to R .. (we're keeping xr and clazzes protected) */
	throwR(msg, xr, clazzes);
	/* should not return, but just in case ... */

	/* protect stack: xobj, clz, msg, xcl, xr + mk_class */
	/* (it may confuse the UP checker since the order does vary by path, but the count is the same) */
	UNPROTECT(6);
}

/* clear any pending exceptions */
HIDE void clx(JNIEnv *env) {
	if (env && (*env)->ExceptionOccurred(env))
		(*env)->ExceptionClear(env);
}

#ifdef JNI_CACHE
HIDE JNIEnv *getJNIEnvSafe(void);
HIDE JNIEnv *getJNIEnv(void) {
  return (use_eenv)?eenv:getJNIEnvSafe();
}

HIDE JNIEnv *getJNIEnvSafe(void)
#else
HIDE JNIEnv *getJNIEnv(void)
#endif
  {
    JNIEnv *env;
    jsize l;
    jint res;

    if (!jvm) { /* we're hoping that the JVM pointer won't change :P we fetch it just once */
        res = JNI_GetCreatedJavaVMs(&jvm, 1, &l);
        if (res != 0) {
            error("JNI_GetCreatedJavaVMs failed! (result:%d)",(int)res); return 0;
        }
        if (l < 1)
	    error("No running JVM detected. Maybe .jinit() would help.");
	if (!rJava_initialized)
	    error("rJava was called from a running JVM without .jinit().");
    }
    res = (*jvm)->AttachCurrentThread(jvm, (void**) &env, 0);
    if (res!=0) {
      error("AttachCurrentThread failed! (result:%d)", (int)res); return 0;
    }
    if (env && !eenv) eenv=env;

    return env;
}

REP void RuseJNICache(int *flag) {
  if (flag) use_eenv=*flag;
}
