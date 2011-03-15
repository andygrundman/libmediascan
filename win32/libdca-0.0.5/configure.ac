dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.54)
AC_INIT([libdca],[0.0.5])
AC_CONFIG_SRCDIR([src/dcadec.c])
AC_CONFIG_AUX_DIR(autotools)
AC_CONFIG_FILES([Makefile autotools/Makefile include/Makefile test/Makefile
    src/Makefile libdca/Makefile libao/Makefile vc++/Makefile
    libdca/libdca.pc libdca/libdts.pc])
AM_INIT_AUTOMAKE([1.5 dist-bzip2 no-dist-gzip check-news -Wall])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE
AC_CANONICAL_HOST

dnl Checks for compiler
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_GCC_TRADITIONAL

dnl CC-specific flags
AC_SUBST([OPT_CFLAGS])
if test x"$GCC" = x"yes"; then

    dnl GCC-specific flags

    dnl -Wall
    dnl -Werror moved to the end to not disturb the configure script
    TRY_CFLAGS="$OPT_CFLAGS -Wall"
    AC_TRY_CFLAGS([$TRY_CFLAGS $CFLAGS],[OPT_CFLAGS=$TRY_CFLAGS])

    dnl -O3
    changequote(<<,>>)
    TRY_CFLAGS=`echo "$OPT_CFLAGS $CFLAGS"|sed "s/-O[0-9]*//g"`
    changequote([,])
    TRY_CFLAGS="$TRY_CFLAGS -O3"
    AC_TRY_CFLAGS([$TRY_CFLAGS],[OPT_CFLAGS=$TRY_CFLAGS; CFLAGS=""])

    AC_ARG_ENABLE([debug],
	[  --enable-debug          debug mode configuration])
    if test x"$enable_debug" = x"yes"; then
	AC_DEFINE([DEBUG],,[debug mode configuration])
    else
	dnl -fomit-frame-pointer
	TRY_CFLAGS="$OPT_CFLAGS -fomit-frame-pointer"
	AC_TRY_CFLAGS([$TRY_CFLAGS $CFLAGS],[OPT_CFLAGS=$TRY_CFLAGS])
    fi

    dnl arch-specific flags
    case "$host" in
    i?86-* | k?-*)
	case "$host" in
	i386-*) TRY_CFLAGS="$OPT_CFLAGS -mtune=i386";;
	i486-*) TRY_CFLAGS="$OPT_CFLAGS -mtune=i486";;
	i586-*) TRY_CFLAGS="$OPT_CFLAGS -mtune=pentium";;
	i686-*) TRY_CFLAGS="$OPT_CFLAGS -mtune=pentiumpro";;
	k6-*)   TRY_CFLAGS="$OPT_CFLAGS -mtune=k6";;
	esac
	AC_TRY_CFLAGS([$TRY_CFLAGS $CFLAGS],[OPT_CFLAGS=$TRY_CFLAGS]);;
    sparc-* | sparc64-*)
	TRY_CFLAGS="$OPT_CFLAGS -mtune=ultrasparc"
	AC_TRY_CFLAGS([$TRY_CFLAGS $CFLAGS],[OPT_CFLAGS=$TRY_CFLAGS]);;
    esac
elif test x"$CC" = x"tendracc"; then
    dnl TenDRA portability checking compiler
    TENDRA=yes
    OPT_CFLAGS="-Xp -Yansi -f`pwd`/include/tendra.h -DELIDE_CODE"
    enable_mlib=no
    enable_oss=no
    enable_solaris_audio=no
elif test x"$CC" = x"icc" -a x"`$CC -V 2>&1 | grep Intel`" != x""; then
    dnl Intel C++ compiler
    OPT_CFLAGS="-g -O3 -unroll -ip"; CFLAGS=""
else
    dnl non-gcc flags - we probably need exact configuration triplets here.
    case "$host" in
    sparc-sun-solaris*)
	TRY_CFLAGS="$OPT_CFLAGS -xCC -fast -xO5"
	AC_TRY_CFLAGS([$TRY_CFLAGS $CFLAGS],[OPT_CFLAGS=$TRY_CFLAGS]);;
    esac
fi

# dnl Checks for libtool - this must be done after we set cflags
# AC_LIBTOOL_WIN32_DLL
m4_undefine([AC_PROG_CXX])
m4_defun([AC_PROG_CXX],[])
m4_undefine([AC_PROG_F77])
m4_defun([AC_PROG_F77],[])
AC_PROG_LIBTOOL

dnl Checks for libraries.
AC_CHECK_LIB(m,cos,[
    LIBAO_LIBS="-lm"
    AC_SUBST([LIBAO_LIBS])
    LIBDCA_LIBS="-lm"
    AC_SUBST([LIBDCA_LIBS])
    COMPARE_LIBS="-lm"
    AC_SUBST([COMPARE_LIBS])
])

dnl Checks for header files.
AC_CHECK_HEADERS([sys/time.h time.h sys/timeb.h io.h])
AC_CHECK_TYPES([struct timeval], [], [],
    [#include <sys/time.h>
    #include <time.h>])
AC_CHECK_GENERATE_INTTYPES([include])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_ALWAYS_INLINE
AC_C_RESTRICT
AC_C_BUILTIN_EXPECT
AC_TYPE_SIZE_T
AC_C_BIGENDIAN

dnl Checks for library functions.
AC_CHECK_FUNCS([memalign gettimeofday ftime])
AC_TYPE_SIGNAL

builtin([include],[libdca/configure.incl])
builtin([include],[libao/configure.incl])

AC_ARG_ENABLE([warnings],
    [  --enable-warnings       treat warnings as errors])
if test x"$enable_warnings" = x"yes" -a x"$GCC" = x"yes"; then
    dnl compiler warnings
    TRY_CFLAGS="$OPT_CFLAGS -Werror"
    AC_TRY_CFLAGS([$TRY_CFLAGS $CFLAGS],[OPT_CFLAGS=$TRY_CFLAGS])
elif test x"$TENDRA" = x"yes"; then
    dnl TenDRA portability checking compiler
    OPT_CFLAGS="$OPT_CFLAGS -DTenDRA_check"
fi

builtin([include],[src/configure.incl])

AC_C_ATTRIBUTE_ALIGNED

AC_OUTPUT
