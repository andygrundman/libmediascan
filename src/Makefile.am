lib_LTLIBRARIES = libmediascan.la

libmediascan_la_SOURCES = buffer.c mediascan.c progress.c result.c error.c video.c util.c \
  libdlna/audio_aac.c libdlna/audio_ac3.c libdlna/audio_amr.c libdlna/audio_atrac3.c \
  libdlna/audio_g726.c libdlna/audio_lpcm.c libdlna/audio_mp2.c libdlna/audio_mp3.c \
  libdlna/audio_wma.c libdlna/av_mpeg1.c libdlna/av_mpeg2.c libdlna/av_mpeg4_part10.c \
  libdlna/av_mpeg4_part2.c libdlna/av_wmv9.c libdlna/containers.c libdlna/image_jpeg.c \
  libdlna/image_png.c libdlna/profiles.c
  
libmediascan_la_CFLAGS = -I$(top_srcdir)/include

libmediascan_la_LDFLAGS = -version-info 0:0:0
