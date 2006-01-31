# Process this file with autoconf to produce a configure script.
AC_INIT(rJava, 0.1, Simon.Urbanek@math.uni-augsburg.de)
AC_CONFIG_SRCDIR([src/rJava.c])
AC_CONFIG_HEADER([src/config.h])

# find R home and set CC/CFLAGS
: ${R_HOME=`R RHOME`}
if test -z "${R_HOME}"; then
  echo "could not determine R_HOME"
  exit 1
fi
CC=`${R_HOME}/bin/R CMD config CC`;
CFLAGS=`${R_HOME}/bin/R CMD config CFLAGS`
AC_SUBST(R_HOME)

## enable threads, i.e. Java is running is a separate thread
AC_ARG_ENABLE([threads],
[AC_HELP_STRING([--enable-threads],
		[enable the use of threads, i.e. Java is run on a separate thread.
		This is necessary for some implementations of AWT. This feature
		is highly experimental, becasue of synchronization issues, so
		use with care. @<:@no@:>@])],
[want_threads="${enableval}"],
[want_threads=no])

## enable JNI-cache
AC_ARG_ENABLE([jni-cache],
[AC_HELP_STRING([--enable-jni-cache],
	        [enable support for caching of the JNI environment. With this
		 option turned on, the JNI state is stored locally and re-used
		 for subsequent calls. This will work *only* if no threads are
		 used, because each thread has a separate JNI state. Enabling
		 this option can give some performance boost for applications
		 that call JNI very often. If used in a threaded environment,
		 it is bound to crash, so use with care. @<:@no@:>@])],
[want_jni_cache="${enableval}"],
[want_jni_cache=no])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([memset mkdir rmdir select socket])

AC_MSG_CHECKING([endianess])
AC_TRY_RUN(
	[
int main(void) { int i=0x12345678; return (*((char*)&i)==0x78)?0:1; }
	],
	[AC_MSG_RESULT(intel-like)],
	[
		AC_MSG_RESULT(PPC-like)
		AC_DEFINE(SWAPEND, 1, [Must be defined for platforms with bytesex inverse to intel style])
	]
)

if test -z "${JAVA_HOME}" ; then
  JAVA_PATH=${JAVA_HOME}:${PATH}
else
  JAVA_PATH=${PATH}
fi
JAVA_PATH=${JAVA_PATH}:/usr/java/bin:/usr/jdk/bin:/usr/lib/java/bin:/usr/lib/jdk/bin:/usr/local/java/bin:/usr/local/jdk/bin:/usr/local/lib/java/bin:/usr/local/lib/jdk/bin
AC_PATH_PROGS(JAVA,java,,${JAVA_PATH})

AC_MSG_CHECKING([Java environment home])

if test -z "${JAVA_HOME}" ; then
  JAVA_HOME=`${JAVA} -classpath . getsp java.home`
fi

if test -z "${JAVA_HOME}" ; then
  AC_MSG_ERROR([Cannot find any Java environment. Please install JDK and/or set JAVA_HOME correspondingly.])
fi

AC_MSG_RESULT([in ${JAVA_HOME}])

AC_CHECK_FILE(${JAVA_HOME}/include/jni.h,
 [JNI_H="${JAVA_HOME}/include"],
 [AC_CHECK_FILE(${JAVA_HOME}/jni.h,
  [JNI_H="${JAVA_HOME}"],
  [AC_CHECK_FILE(${JAVA_HOME}/../include/jni.h,
   [JNI_H="${JAVA_HOME}/../include"],
   [AC_MSG_ERROR([jni headers not found. Please make sure you have a proper JDK installed.])
  ])
 ])
])

JAVA_INC="-I${JNI_H}"

# Sun's JDK needs jni_md.h in in addition to jni.h and unfortunately it's stored somewhere else ...
# this should be become more general at some point - so far we're checking linux and solaris only
# (well, those are presumably the only platforms supported by Sun's JDK and others don't need this
# at least as of now - 01/2004)
jac_found_md=no
for mddir in . linux solaris ppc irix alpha aix hp-ux genunix cygwin win32; do
AC_CHECK_FILE(${JNI_H}/$mddir/jni_md.h,[JAVA_INC="${JAVA_INC} -I${JNI_H}/$mddir" jac_found_md=yes])
if test $jac_found_md == yes; then break; fi
done

JAVA_LIBS=`${JAVA} -classpath . getsp -libs`
JAVA_LIBS="${JAVA_LIBS} -ljvm"

if test `uname` = "Darwin" ; then
  JAVA_LIBS="-framework JavaVM"
fi

LIBS="${LIBS} ${JAVA_LIBS}"
CFLAGS="${CFLAGS} ${JAVA_INC}"

SAVE_LDLP="${LD_LIBRARY_PATH}"
JAVA_LD=`${JAVA} getsp java.library.path`
LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${JAVA_LD}"

AC_MSG_CHECKING([whether JNI programs can be compiled])
AC_LINK_IFELSE([
#include <jni.h>
int main(void) {
    jobject o;
    return 0;
}
	],[AC_MSG_RESULT(yes)],
	[AC_MSG_ERROR([Cannot compile a simple JNI program. See config.log for details.])])

AC_MSG_CHECKING([JNI data types])
AC_TRY_RUN(
	[
#include <jni.h>
int main(void) {
  return (sizeof(int)==sizeof(jint) && sizeof(long)==sizeof(long) && sizeof(jbyte)==sizeof(char) && sizeof(jshort)==sizeof(short) && sizeof(jfloat)==sizeof(float) && sizeof(jdouble)==sizeof(double))?0:1;
}
	],
	[AC_MSG_RESULT([ok])],
	[AC_MSG_ERROR([One or more JNI types differ from the corresponding native type. You may need to use non-standard compiler flags or a different compiler in order to fix this.])])

AC_SUBST(JAVA_LIBS)
AC_SUBST(JAVA_INC)
AC_SUBST(JAVA_HOME)
AC_SUBST(JAVA_LD)

use_threads=no
AC_MSG_CHECKING([whether threads support is requested])
if test "${want_threads}" == yes; then
  AC_MSG_RESULT([yes])
  AC_MSG_CHECKING([whether threads can be enabled])
  # check whether we can add THREADS support
  UNAME=`uname`
  # we don't want to run full AC_CANONICAL_HOST, all we care about is OS X
  if test "x$UNAME" == "xDarwin"; then
    use_threads=yes
    AC_DEFINE(THREADS, 1, [Set if threading support should be enabled.])
  fi
  AC_MSG_RESULT([${use_threads}])
else
  AC_MSG_RESULT([no])
fi

AC_MSG_CHECKING([whether JNI cache support is requested])
if test "${want_jni_cache}" == yes; then
  AC_MSG_RESULT([yes])
  if test "${use_threads}" == yes; then
    AC_MSG_ERROR([Threads and JNI cache cannot be used at the same time, because JNI cache is by definition not thread-safe. Please disable either option.])
  fi
  AC_DEFINE(JNI_CACHE, 1, [Set if caching JNI environment is enabled.])
else
  AC_MSG_RESULT([no])
fi

AC_CONFIG_FILES([src/Makevars])
AC_CONFIG_FILES([R/zzz.R])
AC_OUTPUT
