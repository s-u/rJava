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
AC_CHECK_FILE(${JNI_H}/linux/jni_md.h,[JAVA_INC="${JAVA_INC} -I${JNI_H}/linux"])
AC_CHECK_FILE(${JNI_H}/solaris/jni_md.h,[JAVA_INC="${JAVA_INC} -I${JNI_H}/solaris"])

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

AC_CONFIG_FILES([src/Makevars])
AC_CONFIG_FILES([R/zzz.R])
AC_OUTPUT
