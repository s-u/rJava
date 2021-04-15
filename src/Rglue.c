#define USE_RINTERNALS 1
#include "rJava.h"
#include <R.h>
#include <Rdefines.h>
#include <R_ext/Parse.h>
#include <R_ext/Print.h>
#include <R_ext/Riconv.h>
#include <errno.h>

/* R 4.0.1 broke EXTPTR_PTR ABI so re-map it to safety at
   the small expense of speed */
#ifdef  EXTPTR_PTR
#undef  EXTPTR_PTR
#endif
#define EXTPTR_PTR(X) R_ExternalPtrAddr(X)
/* PROT/TAG are safe so far, but just to make sure ... */
#ifdef  EXTPTR_PROT
#undef  EXTPTR_PROT
#endif
#define EXTPTR_PROT(X) R_ExternalPtrProtected(X)
#ifdef  EXTPTR_TAG
#undef  EXTPTR_TAG
#endif
#define EXTPTR_TAG(X) R_ExternalPtrTag(X)

#include <stdarg.h>

/* max supported # of parameters to Java methods */
#ifndef maxJavaPars
#define maxJavaPars 32
#endif

/* pre-2.4 have no S4SXP but used VECSXP instead */
#ifndef S4SXP
#define S4SXP VECSXP
#endif

/** returns TRUE if JRI has callback support compiled in or FALSE otherwise */
REPC SEXP RJava_has_jri_cb() {
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
HIDE void rjprintf(char *fmt, ...) {
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

HIDE long time_ms() {
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
HIDE void profReport(char *fmt, ...) {
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

static void JRefObjectFinalizer(SEXP ref) {
    if (java_is_dead) return;

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

#if R_VERSION >= R_Version(2,7,0)
/* returns string from a CHARSXP making sure that the result is in UTF-8
   NOTE: this should NOT be used to create Java strings as they require UTF-16 natively */
const char *rj_char_utf8(SEXP s) {
    return (Rf_getCharCE(s) == CE_UTF8) ? CHAR(s) : Rf_reEnc(CHAR(s), getCharCE(s), CE_UTF8, 0); /* subst. invalid chars: 1=hex, 2=., 3=?, other=skip */
}

#ifdef WIN32
extern unsigned int localeCP;
static char cpbuf[16];
#endif
static jchar js_zero[2] = { 0, 0 };
static jchar js_buf[128];
/* returns string from a CHARSXP making sure that the result is in UTF-16.
   the buffer is owned by the function and may be static, so copy after use */
int rj_char_utf16(SEXP s, jchar **buf) {
    void *ih;
    cetype_t ce_in = getCharCE(s);
    const char *ifrom = "", *c = CHAR(s), *ce = strchr(c, 0);
    if (ce == c) {
	buf[0] = js_zero;
	return 0;
    }
    size_t osize = sizeof(jchar) * (ce - c + 1), isize = ce - c;
    jchar *js = buf[0] = (osize < sizeof(js_buf)) ? js_buf : (jchar*) R_alloc(sizeof(jchar), ce - c + 1);
    char *dst = (char*) js;
    int end_test = 1;

    switch (ce_in) {
#ifdef WIN32
    case CE_NATIVE:
	sprintf(cpbuf, "CP%d", localeCP);
	ifrom = cpbuf;
	break;
    case CE_LATIN1: ifrom = "CP1252"; break;
#else
    case CE_LATIN1: ifrom = "latin1"; break;
#endif
    default:
	ifrom = "UTF-8"; break;
    }

    ih = Riconv_open(((char*)&end_test)[0] == 1 ? "UTF-16LE" : "UTF-16BE", ifrom);
    if(ih == (void *)(-1))
	Rf_error("Unable to start conversion to UTF-16");
    while (c < ce) {
	size_t res = Riconv(ih, &c, &isize, &dst, &osize);
	/* this should never happen since we allocated far more than needed */
	if (res == -1 && errno == E2BIG)
	    Rf_error("Conversion to UTF-16 failed due to unexpectedly large buffer requirements.");
	else if(res == -1 && (errno == EILSEQ || errno == EINVAL)) { /* invalid char */
	    *(dst++) = '?';
	    *(dst++) = 0;
	    osize -= 2;
	    c++;
	    isize--;
	}
    }
    Riconv_close(ih);
    return dst - (char*) js;
}

/* Java returns *modified* UTF-8 which is incompatible with UTF-8,
   so we have to detect the illegal surrgoate pairs and convert them */
SEXP mkCharUTF8(const char *src) {
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
	    if (!dst)
		Rf_error("Cannot allocate memory for surrogate pair conversion");
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
		    Rf_error("illegal 2-byte sequence in Java string");
		}
		*(d++) = *(c++);
		continue;
	    }
	    if ((c[-1] & 0xF0) != 0xE0) { /* must be 3-byte */
		if (dst) free(dst);
		Rf_error("illegal multi-byte seqeunce in Java string (>3-byte)");
	    }
	    if (((c[0] & 0xC0) != 0x80 ||
		 (c[1] & 0xC0) != 0x80)) {
		if (dst) free(dst);
		Rf_error("illegal 3-byte sequence in Java string");
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
		Rf_error("illegal sequence in Java string: low surrogate pair without a high one");
	    }
	    c += 2; /* move to the low pair */
	    if (c[0] != 0xED ||
		(c[1] & 0xF0) != 0xB0 ||
		(c[2] & 0xC0) != 0x80) {
		if (dst) free(dst);
		Rf_error("illegal sequence in Java string: high surrogate pair not followed by low one");
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

#endif

static jstring newJavaString(JNIEnv *env, SEXP sChar) {
    jchar *s;
    size_t len = rj_char_utf16(sChar, &s);
    return newString16(env, s, (len + 1) >> 1);
}

HIDE void deserializeSEXP(SEXP o) {
  _dbg(rjprintf("attempt to deserialize %p (clCl=%p, oCL=%p)\n", o, clClassLoader, oClassLoader));
  SEXP s = EXTPTR_PROT(o);
  if (TYPEOF(s) == RAWSXP && EXTPTR_PTR(o) == NULL) {
    JNIEnv *env = getJNIEnv();
    if (env && clClassLoader && oClassLoader) {
      jbyteArray ser = newByteArray(env, RAW(s), LENGTH(s));
      if (ser) {
	jmethodID mid = (*env)->GetMethodID(env, clClassLoader, "toObject", "([B)Ljava/lang/Object;");
	if (mid) {
	  jobject res = (*env)->CallObjectMethod(env, oClassLoader, mid, ser);
	  if (res) {
	    jobject go = makeGlobal(env, res);
	    _mp(MEM_PROF_OUT("R %08x RNEW %08x\n", (int) go, (int) res))
	    if (go) {
	      _dbg(rjprintf(" - succeeded: %p\n", go));
	      /* set the deserialized object */
	      R_SetExternalPtrAddr(o, go);
	      /* Note: currently we don't remove the serialized content, because it was created explicitly using .jcache to allow repeated saving. Once this is handled by a hook, we shall remove it. However, to assure compatibility TAG is always NULL for now, so we do clear the cache if TAG is non-null for future use. */
	      if (EXTPTR_TAG(o) != R_NilValue) {
		/* remove the serialized raw vector */
		SETCDR(o, R_NilValue); /* Note: this is abuse of the API since it uses the fact that PROT is stored in CDR */
	      }
	    }
	  }
	}
	releaseObject(env, ser);
      }
    }
  }
}

#define addtmpo(T, X) { jobject _o = X; if (_o) { _dbg(rjprintf(" parameter to release later: %lx\n", (unsigned long) _o)); *T=_o; T++;} }
#define fintmpo(T) { *T = 0; }

/* concatenate a string to a signature buffer increasing it as necessary */
static void strcats(sig_buffer_t *sig, const char *add) {
  int l = strlen(add);
  int al = sig->len;
  if (al + l >= sig->maxsig - 1) {
    sig->maxsig += 8192;
    if (sig->sig == sig->sigbuf) { /* first-time allocation */
      char *ns = (char*) malloc(sig->maxsig);
      memcpy(ns, sig->sig, al + 1);
      sig->sig = ns;
    } else /* re-allocation */
      sig->sig = (char*) realloc(sig->sig, sig->maxsig);
  }
  strcpy(sig->sig + al, add);
  sig->len += l;
}

/* call strcats() but also convert class names to JNI notation */
static void strcats_conv(sig_buffer_t *sig, const char *add) {
  int ol = sig->len, nl;
  strcats(sig, add);
  nl = sig->len;
  while (ol < nl) {
    if (sig->sig[ol] == '.')
      sig->sig[ol] = '/';
    ol++;
  }
}

/* initialize a signature buffer */
HIDE void init_sigbuf(sig_buffer_t *sb) {
  sb->len = 0;
  sb->maxsig = sizeof(sb->sigbuf);
  sb->sig = sb->sigbuf;
}

/* free the content of a signature buffer (if necessary) */
HIDE void done_sigbuf(sig_buffer_t *sb) {
  if (sb->sig != sb->sigbuf) free(sb->sig);
}

/** converts parameters in SEXP list to jpar and sig.
    since 0.4-4 we ignore named arguments in par
*/
static int Rpar2jvalue(JNIEnv *env, SEXP par, jvalue *jpar, sig_buffer_t *sig, int maxpars, jobject *tmpo) {
  SEXP p=par;
  SEXP e;
  int jvpos=0;
  int i=0;

  while (p && TYPEOF(p)==LISTSXP && (e=CAR(p))) {
    /* skip all named arguments */
    if (TAG(p) && TAG(p)!=R_NilValue) { p=CDR(p); continue; };

    if (jvpos >= maxpars) {
	if (maxpars == maxJavaPars)
	    Rf_error("Too many arguments in Java call. maxJavaPars is %d, recompile rJava with higher number if needed.", maxJavaPars);
	break;
    }

    _dbg(rjprintf("Rpar2jvalue: par %d type %d\n",i,TYPEOF(e)));
    if (TYPEOF(e)==STRSXP) {
      _dbg(rjprintf(" string vector of length %d\n",LENGTH(e)));
      if (LENGTH(e)==1) {
	  SEXP sv = STRING_ELT(e, 0);
	  strcats(sig,"Ljava/lang/String;");
	  if (sv == R_NaString) {
	      addtmpo(tmpo, jpar[jvpos++].l = 0);
	  } else {
	      addtmpo(tmpo, jpar[jvpos++].l = newJavaString(env, sv));
	  }
      } else {
	  int j = 0;
	  jobjectArray sa = (*env)->NewObjectArray(env, LENGTH(e), javaStringClass, 0);
	  _mp(MEM_PROF_OUT("  %08x LNEW string[%d]\n", (int) sa, LENGTH(e)))
	  if (!sa) {
	      fintmpo(tmpo);
	      error("unable to create string array.");
	      return -1;
	  }
	  addtmpo(tmpo, sa);
	  while (j < LENGTH(e)) {
	      SEXP sv = STRING_ELT(e,j);
	      if (sv == R_NaString) {
	      } else {
		  jobject s = newJavaString(env, sv);
		  _dbg(rjprintf (" [%d] \"%s\"\n",j,CHAR_UTF8(sv)));
		  (*env)->SetObjectArrayElement(env, sa, j, s);
		  if (s) releaseObject(env, s);
	      }
	      j++;
	  }
	  jpar[jvpos++].l = sa;
	  strcats(sig,"[Ljava/lang/String;");
      }
    } else if (TYPEOF(e)==RAWSXP) {
      _dbg(rjprintf(" raw vector of length %d\n", LENGTH(e)));
      strcats(sig,"[B");
      addtmpo(tmpo, jpar[jvpos++].l=newByteArray(env, RAW(e), LENGTH(e)));
    } else if (TYPEOF(e)==INTSXP) {
      _dbg(rjprintf(" integer vector of length %d\n",LENGTH(e)));
      if (LENGTH(e)==1) {
	if (inherits(e, "jbyte")) {
	  _dbg(rjprintf("  (actually a single byte 0x%x)\n", INTEGER(e)[0]));
	  jpar[jvpos++].b=(jbyte)(INTEGER(e)[0]);
	  strcats(sig,"B");
	} else if (inherits(e, "jchar")) {
	  _dbg(rjprintf("  (actually a single character 0x%x)\n", INTEGER(e)[0]));
	  jpar[jvpos++].c=(jchar)(INTEGER(e)[0]);
	  strcats(sig,"C");
	} else if (inherits(e, "jshort")) {
	  _dbg(rjprintf("  (actually a single short 0x%x)\n", INTEGER(e)[0]));
	  jpar[jvpos++].s=(jshort)(INTEGER(e)[0]);
	  strcats(sig,"S");
	} else {
	  strcats(sig,"I");
	  jpar[jvpos++].i=(jint)(INTEGER(e)[0]);
	  _dbg(rjprintf("  single int orig=%d, jarg=%d [jvpos=%d]\n",
		   (INTEGER(e)[0]),
		   jpar[jvpos-1],
		   jvpos));
	}
      } else {
	if (inherits(e, "jbyte")) {
	  strcats(sig,"[B");
	  addtmpo(tmpo, jpar[jvpos++].l=newByteArrayI(env, INTEGER(e), LENGTH(e)));
	} else if (inherits(e, "jchar")) {
	  strcats(sig,"[C");
	  addtmpo(tmpo, jpar[jvpos++].l=newCharArrayI(env, INTEGER(e), LENGTH(e)));
	} else if (inherits(e, "jshort")) {
	  strcats(sig,"[S");
	  addtmpo(tmpo, jpar[jvpos++].l=newShortArrayI(env, INTEGER(e), LENGTH(e)));
	} else {
	  strcats(sig,"[I");
	  addtmpo(tmpo, jpar[jvpos++].l=newIntArray(env, INTEGER(e), LENGTH(e)));
	}
      }
    } else if (TYPEOF(e)==REALSXP) {
      if (inherits(e, "jfloat")) {
	_dbg(rjprintf(" jfloat vector of length %d\n", LENGTH(e)));
	if (LENGTH(e)==1) {
	  strcats(sig,"F");
	  jpar[jvpos++].f=(jfloat)(REAL(e)[0]);
	} else {
	  strcats(sig,"[F");
	  addtmpo(tmpo, jpar[jvpos++].l=newFloatArrayD(env, REAL(e),LENGTH(e)));
	}
      } else if (inherits(e, "jlong")) {
	_dbg(rjprintf(" jlong vector of length %d\n", LENGTH(e)));
	if (LENGTH(e)==1) {
	  strcats(sig,"J");
	  jpar[jvpos++].j=(jlong)(REAL(e)[0]);
	} else {
	  strcats(sig,"[J");
	  addtmpo(tmpo, jpar[jvpos++].l=newLongArrayD(env, REAL(e),LENGTH(e)));
	}
      } else {
	_dbg(rjprintf(" real vector of length %d\n",LENGTH(e)));
	if (LENGTH(e)==1) {
	  strcats(sig,"D");
	  jpar[jvpos++].d=(jdouble)(REAL(e)[0]);
	} else {
	  strcats(sig,"[D");
	  addtmpo(tmpo, jpar[jvpos++].l=newDoubleArray(env, REAL(e),LENGTH(e)));
	}
      }
    } else if (TYPEOF(e)==LGLSXP) {
      _dbg(rjprintf(" logical vector of length %d\n",LENGTH(e)));
      if (LENGTH(e)==1) {
	strcats(sig,"Z");
	jpar[jvpos++].z=(jboolean)(LOGICAL(e)[0]);
      } else {
	strcats(sig,"[Z");
	addtmpo(tmpo, jpar[jvpos++].l=newBooleanArrayI(env, LOGICAL(e),LENGTH(e)));
      }
    } else if (TYPEOF(e)==VECSXP || TYPEOF(e)==S4SXP) {
      if (TYPEOF(e) == VECSXP)
	_dbg(rjprintf(" generic vector of length %d\n", LENGTH(e)));
      if (inherits(e, "jclassName")) {
	_dbg(rjprintf(" jclassName, replacing with embedded class jobjRef"));
	e = GET_SLOT(e, install("jobj"));
      }
      if (IS_JOBJREF(e)) {
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
	    jc=CHAR_UTF8(STRING_ELT(sclass,0));
	  if (IS_JARRAYREF(e) && jc && !*jc) {
	    /* if it's jarrayRef with jclass "" then it's an uncast array - use sig instead */
	    sclass=GET_SLOT(e, install("jsig"));
	    if (sclass && TYPEOF(sclass)==STRSXP && LENGTH(sclass)==1)
	      jc=CHAR_UTF8(STRING_ELT(sclass,0));
	  }
	}
	if (jc) {
	  if (*jc!='[') { /* not an array, we assume it's an object of that class */
	    strcats(sig,"L"); strcats_conv(sig,jc); strcats(sig,";");
	  } else /* array signature is passed as-is */
	    strcats_conv(sig,jc);
	} else
	  strcats(sig,"Ljava/lang/Object;");
	jpar[jvpos++].l=o;
      } else {
	_dbg(rjprintf(" (ignoring)\n"));
      }
    }
    i++;
    p=CDR(p);
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

/** map one parameter into jvalue and determine its signature (unused in fields.c) */
HIDE jvalue R1par2jvalue(JNIEnv *env, SEXP par, sig_buffer_t *sig, jobject *otr) {
  jobject tmpo[4] = {0, 0};
  jvalue v[4];
  int p = Rpar2jvalue(env, CONS(par, R_NilValue), v, sig, 2, tmpo);
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
REPE SEXP RcallMethod(SEXP par) {
  SEXP p = par, e;
  sig_buffer_t sig;
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
    clnam = CHAR_UTF8(STRING_ELT(e, 0));
  else
    error_return("RcallMethod: invalid object parameter");
  if (!o && !clnam)
    error_return("RcallMethod: attempt to call a method of a NULL object.");
#ifdef RJ_DEBUG
  {
    SEXP de=CAR(CDR(p));
    rjprintf("RcallMethod (env=%x):\n",env);
    if (TYPEOF(de)==STRSXP && LENGTH(de)>0)
      rjprintf(" method to call: %s on object 0x%x or class %s\n",CHAR_UTF8(STRING_ELT(de,0)),o,clnam);
  }
#endif
  if (clnam)
    cls = findClass(env, clnam, oClassLoader);
  else
    cls = objectClass(env,o);
  if (!cls)
    error_return("RcallMethod: cannot determine object class");
#ifdef RJ_DEBUG
  rjprintf(" class: "); printObject(env, cls);
#endif
  e=CAR(p); p=CDR(p);
  if (TYPEOF(e)==STRSXP && LENGTH(e)==1) { /* signature */
    retsig=CHAR_UTF8(STRING_ELT(e,0));
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
  mnam = CHAR_UTF8(STRING_ELT(e,0));
  init_sigbuf(&sig);
  strcats(&sig, "(");
  Rpar2jvalue(env, p, jpar, &sig, maxJavaPars, tmpo);
  strcats(&sig, ")");
  strcats(&sig, retsig);
  _dbg(rjprintf(" method \"%s\" signature is %s\n", mnam, sig.sig));
  mid=o?
    (*env)->GetMethodID(env, cls, mnam, sig.sig):
    (*env)->GetStaticMethodID(env, cls, mnam, sig.sig);
  if (!mid && o) { /* try static method as a fall-back */
    checkExceptionsX(env, 1);
    o = 0;
    mid = (*env)->GetStaticMethodID(env, cls, mnam, sig.sig);
  }
  if (!mid) {
    checkExceptionsX(env, 1);
    Rfreejpars(env, tmpo);
    releaseObject(env, cls);
    done_sigbuf(&sig);
    error("method %s with signature %s not found", mnam, sig.sigbuf);
  }
#if (RJ_PROFILE>1)
  profReport("Found CID/MID for %s %s:",mnam,sig.sig);
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
  error("unsupported/invalid method signature %s", retsig);
  return R_NilValue;
}

/** like RcallMethod but the call will be synchronized */
REPE SEXP RcallSyncMethod(SEXP par) {
  SEXP p=par, e;
  jobject o = 0;
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
REPE SEXP RcreateObject(SEXP par) {
  SEXP p=par;
  SEXP e;
  int silent=0;
  const char *class;
  sig_buffer_t sig;
  jvalue jpar[maxJavaPars];
  jobject tmpo[maxJavaPars+1];
  jobject o, loader = 0;
  JNIEnv *env=getJNIEnv();

  if (TYPEOF(p)!=LISTSXP) {
    _dbg(rjprintf("Parameter list expected but got type %d.\n",TYPEOF(p)));
    error_return("RcreateObject: invalid parameter");
  }

  p=CDR(p); /* skip first parameter which is the function name */
  e=CAR(p); /* second is the class name */
  if (TYPEOF(e)!=STRSXP || LENGTH(e)!=1)
    error("RcreateObject: invalid class name");
  class = CHAR_UTF8(STRING_ELT(e,0));
  _dbg(rjprintf("RcreateObject: new object of class %s\n",class));
  init_sigbuf(&sig);
  strcats(&sig, "(");
  p=CDR(p);
  Rpar2jvalue(env, p, jpar, &sig, maxJavaPars, tmpo);
  strcats(&sig, ")V");
  _dbg(rjprintf(" constructor signature is %s\n",sig.sig));

  /* look for named arguments */
  while (TYPEOF(p)==LISTSXP) {
    if (TAG(p) && isSymbol(TAG(p))) {
      if (TAG(p)==install("silent") && isLogical(CAR(p)) && LENGTH(CAR(p))==1)
	silent=LOGICAL(CAR(p))[0];

      /* class.loader */
      if (TAG(p)==install("class.loader")) {
	SEXP e = CAR(p);
	if (TYPEOF(e) == S4SXP && IS_JOBJREF(e)) {
	  SEXP sref = GET_SLOT(e, install("jobj"));
	  if (sref && TYPEOF(sref) == EXTPTRSXP) {
	    jverify(sref);
	    loader = (jobject)EXTPTR_PTR(sref);
	  }
	} else if (e != R_NilValue)
	  Rf_error("invalid class.loader");
      }
    }
    p = CDR(p);
  }
  if (!loader) loader = oClassLoader;
BEGIN_RJAVA_CALL
  o = createObject(env, class, sig.sig, jpar, silent, loader);
END_RJAVA_CALL
  done_sigbuf(&sig);
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
  jobject r;
  char cn[128];
  if (!o) return mkString("java/jang/Object");
  cls = objectClass(env, o);
  if (!cls) return mkString("java/jang/Object");
  r = (*env)->CallObjectMethod(env, cls, mid_getName);
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
HIDE SEXP new_jobjRef(JNIEnv *env, jobject o, const char *klass) {
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

/**
 * creates a new jclassName object. similar to what the jclassName
 * function does in the R side
 *
 * @param env pointer to the jni env
 * @param cl Class instance
 */
HIDE SEXP new_jclassName(JNIEnv *env, jobject/*Class*/ cl ) {
  SEXP oo = NEW_OBJECT(MAKE_CLASS("jclassName"));
  if (!inherits(oo, "jclassName"))
    error("unable to create jclassName object");
  PROTECT(oo);
  SET_SLOT(oo, install("name"), getName(env, cl) );
  SET_SLOT(oo, install("jobj"), new_jobjRef( env, cl, "java/lang/Class" ) );
  UNPROTECT(1);
  return oo;
}

/** Calls the Class.getName method and return the result as an R STRSXP */
HIDE SEXP getName( JNIEnv *env, jobject/*Class*/ cl){
	char cn[128];

	jstring r = (*env)->CallObjectMethod(env, cl, mid_getName);

	cn[127]=0; *cn=0;
  	int sl = (*env)->GetStringLength(env, r);
  	if (sl>127) {
  	  error("class name is too long");
  	}
  	if (sl) (*env)->GetStringUTFRegion(env, r, 0, sl, cn);
  	char *c=cn; while(*c) { if (*c=='.') *c='/'; c++; }

	SEXP res = PROTECT( mkString(cn ) );
	releaseObject(env, r);
	UNPROTECT(1); /* res */
	return res;
}

static SEXP new_jarrayRef(JNIEnv *env, jobject a, const char *sig) {
  /* it is too tedious to try to do this in C, so we use 'new' R function instead */
  /* SEXP oo = eval(LCONS(install("new"),LCONS(mkString("jarrayRef"),R_NilValue)), R_GlobalEnv); */
  SEXP oo = NEW_OBJECT(MAKE_CLASS("jarrayRef"));
  /* .. and set the slots in C .. */
  if (! IS_JARRAYREF(oo) )
    error("unable to create an array");
  PROTECT(oo);
  SET_SLOT(oo, install("jobj"), j2SEXP(env, a, 1));
  SET_SLOT(oo, install("jclass"), mkString(sig));
  SET_SLOT(oo, install("jsig"), mkString(sig));
  UNPROTECT(1);
  return oo;
}

/**
 * Creates a reference to a rectangular java array.
 *
 * @param env
 * @param a the java object
 * @param sig signature (class of the array object)
 * @param dim dimension vector
 */
static SEXP new_jrectRef(JNIEnv *env, jobject a, const char *sig, SEXP dim ) {
  /* it is too tedious to try to do this in C, so we use 'new' R function instead */
  /* SEXP oo = eval(LCONS(install("new"),LCONS(mkString("jrectRef"),R_NilValue)), R_GlobalEnv); */
  SEXP oo = NEW_OBJECT(MAKE_CLASS("jrectRef"));
  /* .. and set the slots in C .. */
  if (! IS_JRECTREF(oo) )
    error("unable to create an array");
  PROTECT(oo);
  SET_SLOT(oo, install("jobj"), j2SEXP(env, a, 1));
  SET_SLOT(oo, install("jclass"), mkString(sig));
  SET_SLOT(oo, install("jsig"), mkString(sig));
  SET_SLOT(oo, install("dimension"), dim);

  UNPROTECT(1); /* oo */
  return oo;
}

/* this does not take care of multi dimensional arrays properly */

/**
 * Creates a one dimensional java array
 *
 * @param an R list or vector
 * @param cl the class name
 */
REPC SEXP RcreateArray(SEXP ar, SEXP cl) {
  JNIEnv *env=getJNIEnv();

  if (ar==R_NilValue) return R_NilValue;
  switch(TYPEOF(ar)) {
  case INTSXP:
    {
      if (inherits(ar, "jbyte")) {
	jbyteArray a = newByteArrayI(env, INTEGER(ar), LENGTH(ar));
	if (!a) error("unable to create a byte array");
	return new_jarrayRef(env, a, "[B" ) ;
      } else if (inherits(ar, "jchar")) {
	jcharArray a = newCharArrayI(env, INTEGER(ar), LENGTH(ar));
	if (!a) error("unable to create a char array");
	return new_jarrayRef(env, a, "[C" );
      } else if (inherits(ar, "jshort")) {
	jshortArray a = newShortArrayI(env, INTEGER(ar), LENGTH(ar));
	if (!a) error("unable to create a short integer array");
	return new_jarrayRef(env, a, "[S");
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
      while (i < LENGTH(ar)) {
	  SEXP sa = STRING_ELT(ar, i);
	  if (sa != R_NaString) {
	      jobject so = newJavaString(env, sa);
	      (*env)->SetObjectArrayElement(env, a, i, so);
	      releaseObject(env, so);
	  }
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
	    !inherits(e, "jarrayRef") &&
	    !inherits(e, "jrectRef") )
	  error("Cannot create a Java array from a list that contains anything other than Java object references.");
	i++;
      }
      /* optional class name for the objects contained in the array */
      if (TYPEOF(cl)==STRSXP && LENGTH(cl)>0) {
	const char *cname = CHAR_UTF8(STRING_ELT(cl, 0));
	if (cname) {
	  ac = findClass(env, cname, oClassLoader);
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

/** check whether there is an exception pending and
    return the exception if any (NULL otherwise) */
REPC SEXP RpollException() {
  JNIEnv *env=getJNIEnv();
  jthrowable t;
BEGIN_RJAVA_CALL
  t=(*env)->ExceptionOccurred(env);
END_RJAVA_CALL
  _mp(MEM_PROF_OUT("  %08x LNEW RpollException throwable\n", (int)t))
  return t?j2SEXP(env, t, 1):R_NilValue;
}

/** clear any pending exceptions */
REP void RclearException() {
  JNIEnv *env=getJNIEnv();
BEGIN_RJAVA_CALL
  (*env)->ExceptionClear(env);
END_RJAVA_CALL
}

REPC SEXP javaObjectCache(SEXP o, SEXP what) {
  if (TYPEOF(o) != EXTPTRSXP)
    error("invalid object");
  if (TYPEOF(what) == RAWSXP || what == R_NilValue) {
    /* set PROT to the serialization of NULL */
    SETCDR(o, what);
    return what;
  }
  if (TYPEOF(what) == LGLSXP)
    return EXTPTR_PROT(o);
  error("invalid argument");
  return R_NilValue;
}

REPC SEXP RthrowException(SEXP ex) {
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
