#ifndef RJ_STRING_H__
#define RJ_STRING_H__

#include "jni.h"         /* for jchar */
#include <Rinternals.h>  /* for SEXP */

/* --- API --- */

/* Returns static content for short strings so don't re-use.
   For dynamic strings uses R_alloc */
int rj_char_utf16(const char *c, int len, jchar **buf, const char *ifrom, int can_error);

/* wrappers for above to use with CHARSXP to detect proper ifrom */
int rj_rchar_utf16(SEXP s, jchar **buf);
int rj_rchar_utf16_noerr(SEXP s, jchar **buf);

/* return jstring, but do NOT check exceptions */
jstring rj_newJavaString(JNIEnv *env, SEXP sChar);
jstring rj_newNativeJavaString(JNIEnv *env, const char *str, int len);

/* takes modified UTF-8 from Java, creates CHARSXP with valid UTF8 */
SEXP rj_mkCharUTF8(const char *src);
SEXP rj_mkCharUTF8_noerr(const char *src);

#endif
