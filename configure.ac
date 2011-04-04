#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([libmediascan],[0.1],[andy@hybridized.org])
AM_INIT_AUTOMAKE([-Wall -Werror])
LT_PREREQ([2.4])
LT_INIT

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/mediascan.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC([clang gcc])
AM_PROG_CC_C_O

# Checks for libraries.
AC_SEARCH_LIBS([pthread_create], [pthread])
AC_SEARCH_LIBS([zlibVersion], [z])
AC_SEARCH_LIBS([BZ2_bzDecompressInit], [bz2])
AC_SEARCH_LIBS([jpeg_read_header], [jpeg], [], [], [-ljpeg])
AC_SEARCH_LIBS([exif_loader_new], [exif], [], [], [-lexif])
AC_SEARCH_LIBS([av_freep], [avutil], [], [], [-lz])
AC_SEARCH_LIBS([avcodec_version], [avcodec], [], [], [-lavutil -lz])
AC_SEARCH_LIBS([avformat_version], [avformat], [], [], [-lavcodec -lavutil -lz -lbz2])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h pthread.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT64_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC

# Debugging
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug], [enable debug output @<:@default: no@:>@])],
    [debug=${enableval}], [debug=no])

if test "x${debug}" = xyes; then
    AC_DEFINE([DEBUG], 1, [debug enabled])
fi

AC_CONFIG_FILES([Makefile
                 include/Makefile
                 src/Makefile
                 test/Makefile])
AC_OUTPUT
