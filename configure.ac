#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([libmediascan],[0.1],[andy@hybridized.org])
AM_INIT_AUTOMAKE([-Wall -Werror])
LT_PREREQ([2.4])
LT_INIT

AC_CANONICAL_SYSTEM

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/mediascan.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC([clang gcc])
AC_PROG_OBJC([gcc objcc objc])
AM_PROG_CC_C_O

# Checks for libraries.
AC_SEARCH_LIBS([pthread_create], [pthread])
AC_SEARCH_LIBS([zlibVersion], [z])
AC_SEARCH_LIBS([BZ2_bzDecompressInit], [bz2])
AC_SEARCH_LIBS([jpeg_read_header], [jpeg], [], [], [-ljpeg])
AC_SEARCH_LIBS([png_create_read_struct], [png], [], [], [-lpng])
AC_SEARCH_LIBS([DGifOpen], [gif], [], [], [-lgif])
AC_SEARCH_LIBS([exif_loader_new], [exif], [], [], [-lexif])
AC_SEARCH_LIBS([av_freep], [avutil], [], [], [-lz])
AC_SEARCH_LIBS([avcodec_version], [avcodec], [], [], [-lavutil -lz])
AC_SEARCH_LIBS([avformat_version], [avformat], [], [], [-lavcodec -lavutil -lz -lbz2])
AC_SEARCH_LIBS([sws_scale], [swscale], [], [], [-lswscale])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h pthread.h])

# Check BerkeleyDB lib and headers (stolen from memcachedb's configure.ac)
bdbdir="/usr/local/BerkeleyDB.5.1"
AC_ARG_WITH(bdb,
       [  --with-bdb=PATH         Specify path to BerkeleyDB installation ],
       [
                if test "x$withval" != "xno" ; then
                        bdbdir=$withval
                fi
       ]
)
CFLAGS="-I$bdbdir/include $CFLAGS"

case "$target_os" in
	*darwin*)
AM_CONDITIONAL(LINUX, false)
LDFLAGS="-framework CoreFoundation -framework CoreServices -framework Carbon -L$bdbdir/lib $LDFLAGS"
;;
	*)
AM_CONDITIONAL(LINUX, true)
LDFLAGS="-L$bdbdir/lib $LDFLAGS"
;;
esac	

AC_SEARCH_LIBS([db_create], [db], [] ,[AC_MSG_ERROR(cannot find libdb.so in $bdbdir/lib)])
AC_CHECK_HEADERS([db.h], [] ,[AC_MSG_ERROR(cannot find db.h in $bdbdir/include)])

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
