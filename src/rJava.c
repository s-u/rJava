#include <R.h>
#include <Rdefines.h>
#include "rJava.h"
#include <stdlib.h>
#include <string.h>

/* determine whether eenv chache should be used (has no effect if JNI_CACHE is not set) */
int use_eenv = 1;

/* cached environment. Do NOT use directly! Always use getJNIEnv()! */
JNIEnv *eenv;

/* throw an exception using R condition code */
HIDE void throwR(SEXP msg, SEXP jobj) {
	SEXP cond = PROTECT(allocVector(VECSXP, 3));
	SEXP names = PROTECT(allocVector(STRSXP, 3));
	SEXP cln = PROTECT(allocVector(STRSXP, 3));
	SET_VECTOR_ELT(cond, 0, msg);
	SET_VECTOR_ELT(cond, 1, R_NilValue); /* I see no way to get "call" without hacking RCNTX */
	SET_VECTOR_ELT(cond, 2, jobj);
	SET_STRING_ELT(names, 0, mkChar("message"));
	SET_STRING_ELT(names, 1, mkChar("call"));
	SET_STRING_ELT(names, 2, mkChar("jobj"));
	SET_STRING_ELT(cln, 0, mkChar("Exception"));
	SET_STRING_ELT(cln, 1, mkChar("error"));
	SET_STRING_ELT(cln, 2, mkChar("condition"));
	setAttrib(cond, R_NamesSymbol, names);
	setAttrib(cond, R_ClassSymbol, cln);
	UNPROTECT(2);
	eval(LCONS(install("stop"), CONS(cond, R_NilValue)), R_GlobalEnv);
	UNPROTECT(1);
}

/* check for exceptions and throw them to R level */
HIDE void ckx(JNIEnv *env) {
	SEXP xr, xobj, msg = 0;
	jthrowable x = 0;
	if (env && !(x = (*env)->ExceptionOccurred(env))) return;
	if (!env) {
		env = getJNIEnv();
		if (!env)
			error("Unable to retrieve JVM environment.");
		return ckx(env);
	}
	/* env is valid and an exception occurred */
	/* we create the jobj first, because the exception may in theory disappear after being cleared, yet this can be (also in theory) risky as it uses further JNI calls ... */
	xobj = j2SEXP(env, x, 0);
	(*env)->ExceptionClear(env);
	/* ok, now this is a critical part that we do manually to avoid recursion */
	{
		jclass cls = (*env)->GetObjectClass(env, x);
		if (cls) {
			jmethodID mid = (*env)->GetMethodID(env, cls, "toString", "()Ljava/lang/String;");
			if (mid) {
				jstring s = (jstring)(*env)->CallObjectMethod(env, x, mid);
				if (s) {
					char *c = (*env)->GetStringUTFChars(env, s, 0);
					if (c) {
						msg = mkString(c);
						(*env)->ReleaseStringUTFChars(env, s, c);
					}
				}
			}
			if ((*env)->ExceptionOccurred(env))
				(*env)->ExceptionClear(env);
			(*env)->DeleteLocalRef(env, cls);
		} else (*env)->ExceptionClear(env);
		if (!msg)
			msg = mkString("Java Exception <no description because toString() failed>");
	}
	/* delete the local reference to the exception (jobjRef has a global copy) */
	(*env)->DeleteLocalRef(env, x);
	/* construct the jobjRef */
	xr = PROTECT(NEW_OBJECT(MAKE_CLASS("jobjRef")));
	if (inherits(xr, "jobjRef")) {
		SET_SLOT(xr, install("jclass"), mkString("java/lang/Throwable") /* getObjectClassName(env, x) */);
		SET_SLOT(xr, install("jobj"), xobj);
	}
	/* and off to R .. (we're keeping xr protected) */
	throwR(msg, xr);
	/* throwR never returns so don't even bother ... */
}

/* clear any pending exceptions */
HIDE void clx(JNIEnv *env) {
	if (env && (*env)->ExceptionOccurred(env))
		(*env)->ExceptionClear(env);
}

#ifdef JNI_CACHE
HIDE JNIEnv *getJNIEnvSafe();
HIDE JNIEnv *getJNIEnv() {
  return (use_eenv)?eenv:getJNIEnvSafe();
}

HIDE JNIEnv *getJNIEnvSafe()
#else
HIDE JNIEnv *getJNIEnv()
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
    
    /* if (eenv!=env)
        fprintf(stderr, "Warning! eenv=%x, but env=%x - different environments encountered!\n", eenv, env); */
    return env;
}

REP void RuseJNICache(int *flag) {
  if (flag) use_eenv=*flag;
}
