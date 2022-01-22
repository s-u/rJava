#include "rjstring.h"

#include <string.h>
#include <stdlib.h>
#include <R_ext/Riconv.h>
#include <errno.h>

#ifdef WIN32
/* -- currently unused - was used to mimick reEnc()
   extern unsigned int localeCP; 
   static char cpbuf[16]; */
#endif
static jchar js_zero[2] = { 0, 0 };
static jchar js_buf[128];

/* if len = -1 then c is assumed to be NUL terminated */
int rj_char_utf16(const char *c, int len, jchar **buf, const char *ifrom, int can_error) {
    void *ih;
    const char *ce = (len < 0) ? strchr(c, 0) : (c + len);
    if (ce == c) {
	buf[0] = js_zero;
	return 0;
    }
    size_t osize = sizeof(jchar) * (ce - c + 1), isize = ce - c;
    jchar *js = buf[0] = (osize < sizeof(js_buf)) ? js_buf : (jchar*) R_alloc(sizeof(jchar), ce - c + 1);
    char *dst = (char*) js;
    int end_test = 1, is_le = (((char*)&end_test)[0] == 1) ? 1 : 0;
    if (!ifrom) ifrom = "";

#ifdef DEBUG_ENCODING
    fprintf(stderr, "rJava.rj_char_utf16_native:");
    { const char *c0 = c; while (*c0) fprintf(stderr, " %02x", (int)((unsigned char)*(c0++))); }
    fprintf(stderr, "\n");
#endif

    ih = Riconv_open(is_le ? "UTF-16LE" : "UTF-16BE", ifrom);
    if (ih == (void *)(-1)) {
	if (can_error)
	    Rf_error("Unable to start conversion to UTF-16");
	return -1;
    }
    while (c < ce) {
	size_t res = Riconv(ih, &c, &isize, &dst, &osize);
	/* this should never happen since we allocated far more than needed */
	if (res == -1 && errno == E2BIG) {
	    if (can_error)
		Rf_error("Conversion to UTF-16 failed due to unexpectedly large buffer requirements.");
	    return -1;
	} else if(res == -1 && (errno == EILSEQ || errno == EINVAL)) { /* invalid char */
	    if (is_le) {
		*(dst++) = '?';
		*(dst++) = 0;
	    } else {
		*(dst++) = 0;
		*(dst++) = '?';
	    }
	    osize -= 2;
	    c++;
	    isize--;
	}
    }
    Riconv_close(ih);
#ifdef DEBUG_ENCODING
    { const jchar *j = js; while (j < (const jchar*)dst) fprintf(stderr, " %04x", (unsigned int)*(j++)); }
    fprintf(stderr, "\n");
#endif
    return dst - (char*) js;
}

/* returns string from a CHARSXP making sure that the result is in UTF-16.
   the buffer is owned by the function and may be static, so copy after use.

   Returns the length of the resulting string or -1 on error (if
   can_error is 0).
 */
static int rj_CHARSXP_utf16_(SEXP s, jchar **buf, int can_error) {
    cetype_t ce_in = getCharCE(s);
    const char *ifrom = "", *c = CHAR(s), *ce = strchr(c, 0);
    if (ce == c) {
	buf[0] = js_zero;
	return 0;
    }

    switch (ce_in) {
#ifdef WIN32
    case CE_NATIVE:
/* reEnc uses this, but translateCharUtf8 uses "" so let's go with ""
	sprintf(cpbuf, "CP%d", localeCP);
	ifrom = cpbuf;
*/
	break;
    case CE_LATIN1: ifrom = "CP1252"; break;
#else
    case CE_NATIVE: break; /* is already "" */
    case CE_LATIN1: ifrom = "latin1"; break;
#endif
    default:
	ifrom = "UTF-8"; break;
    }

    return rj_char_utf16(c, ce - c, buf, ifrom, can_error);
}

int rj_rchar_utf16(SEXP s, jchar **buf) { return rj_CHARSXP_utf16_(s, buf, 1); }
int rj_rchar_utf16_noerr(SEXP s, jchar **buf) { return rj_CHARSXP_utf16_(s, buf, 0); }

/* FIXME: we should probably deprecate this as well and use UTF-16 instead.
   The only reason not to is that we would have to fully implement
   a full UTF-16 -> UTF-8 conversion including surrogate pairs ... */

/* Java returns *modified* UTF-8 which is incompatible with UTF-8,
   so we have to detect the illegal surrgoate pairs and convert them */
SEXP rj_mkCharUTF8_(const char *src, int can_error) {
    const unsigned char *s = (const unsigned char*) src;
    const unsigned char *c = (const unsigned char*) s;
    /* check if the string contains any surrogate pairs, i.e.
       Unicode in the range 0xD800-0xDFFF
       We want this to be fast since in 99.99% of cases it will
       be false */
    while (*c) {
	if (c[0] == 0xED &&
	    (c[1] & 0xE0) == 0xA0)
	    break;
	c++;
    }
    if (*c) { /* yes, we have to convert them */
	SEXP res;
	const unsigned char *e = (const unsigned char*) strchr((const char*)s, 0); /* find the end for size */
	unsigned char *dst = 0, *d, sbuf[64];
	if (!e) /* should never occur */
	    return mkChar("");
	/* we use static buffer for small strings and dynamic alloc for large */
	if (e - s >= sizeof(sbuf)) {
	    /* allocate temp buffer since our input is const */
	    d = dst = (unsigned char *) malloc(e - s + 1);
	    if (!dst) {
		if (can_error)
		    Rf_error("Cannot allocate memory for surrogate pair conversion");
		return 0;
	    }
	} else
	    d = (unsigned char *)sbuf;
	if (c - s > 0) {
	    memcpy(d, s, c - s);
	    d += c - s;
	}
	while (*c) {
	    unsigned int u1, u;
	    *(d++) = *(c++);
	    /* start of a sequence ? */
	    if ((c[-1] & 0xC0) != 0xC0)
		continue;
	    if ((c[-1] & 0xE0) == 0xC0)  { /* 2-byte, not a surrogate pair */
		if ((c[0] & 0xC0) != 0x80) {
		    if (dst) free(dst);
		    if (can_error)
			Rf_error("illegal 2-byte sequence in Java string");
		    return 0;
		}
		*(d++) = *(c++);
		continue;
	    }
	    if ((c[-1] & 0xF0) != 0xE0) { /* must be 3-byte */
		if (dst) free(dst);
		if (can_error)
		    Rf_error("illegal multi-byte seqeunce in Java string (>3-byte)");
		return 0;
	    }
	    if (((c[0] & 0xC0) != 0x80 ||
		 (c[1] & 0xC0) != 0x80)) {
		if (dst) free(dst);
		if (can_error)
		    Rf_error("illegal 3-byte sequence in Java string");
		return 0;
	    }
	    u1 = ((((unsigned int)c[-1]) & 0x0F) << 12) |
		 ((((unsigned int)c[0]) & 0x3F) << 6) |
		 (((unsigned int)c[1]) & 0x3F);
	    if (u1 < 0xD800 || u1 > 0xDBFF) { /* not a surrogate pair -> regular copy */
		*(d++) = *(c++);
		*(d++) = *(c++);
		continue;
	    }
	    if (u1 >= 0xDC00 && u1 <= 0xDFFF) { /* low surrogate pair ? */
		if (dst) free(dst);
		if (can_error)
		    Rf_error("illegal sequence in Java string: low surrogate pair without a high one");
		return 0;
	    }
	    c += 2; /* move to the low pair */
	    if (c[0] != 0xED ||
		(c[1] & 0xF0) != 0xB0 ||
		(c[2] & 0xC0) != 0x80) {
		if (dst) free(dst);
		if (can_error)
		    Rf_error("illegal sequence in Java string: high surrogate pair not followed by low one");
		return 0;
	    }
	    /* the actually encoded unicode character */
	    u = ((((unsigned int)c[1]) & 0x0F) << 6) |
		(((unsigned int)c[2]) & 0x3F);
	    u |= (u1 & 0x03FF) << 10;
	    u += 0x10000;
	    c += 3;
	    /* it must be <= 0x10FFFF by design (each surrogate has 10 bits) */
	    d[-1]  = (unsigned char) (((u >> 18) & 0x0F) | 0xF0);
	    *(d++) = (unsigned char) (((u >> 12) & 0x3F) | 0x80);
	    *(d++) = (unsigned char) (((u >> 6) & 0x3F) | 0x80);
	    *(d++) = (unsigned char) ((u & 0x3F) | 0x80);
	}
	res = mkCharLenCE((const char*) (dst ? dst : sbuf), dst ? (d - dst) : (d - sbuf), CE_UTF8);
	if (dst) free(dst);
	return res;
    }
    return mkCharLenCE(src, c - s, CE_UTF8);
}

SEXP rj_mkCharUTF8(const char *src) { return rj_mkCharUTF8_(src, 0); }
SEXP rj_mkCharUTF8_noerr(const char *src) { return rj_mkCharUTF8_(src, 1); }

jstring rj_newJavaString(JNIEnv *env, SEXP sChar) {
    jchar *s;
    int len = rj_rchar_utf16(sChar, &s);
    return (*env)->NewString(env, s, (len + 1) >> 1);
}

jstring rj_newNativeJavaString(JNIEnv *env, const char *str, int len) {
    jchar *s;
    int rlen = rj_char_utf16(str, len, &s, "", 0);
    return (rlen < 0) ? 0 : (*env)->NewString(env, s, (rlen + 1) >> 1);
}
