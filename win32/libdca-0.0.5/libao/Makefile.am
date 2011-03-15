AM_CFLAGS = $(OPT_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_LTLIBRARIES = libao.la
libao_la_SOURCES = audio_out.c audio_out_null.c audio_out_float.c \
		   audio_out_oss.c audio_out_solaris.c audio_out_al.c \
		   audio_out_win.c audio_out_wav.c audio_out_aif.c \
		   audio_out_peak.c convert2s16.c \
		   audio_out_internal.h
libao_la_LIBADD = $(LIBAO_LIBS)

EXTRA_DIST = configure.incl
