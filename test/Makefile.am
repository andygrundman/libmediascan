LMS_INCLUDE = -I$(top_srcdir)/include
LMS_LTLIB = ../src/libmediascan.la

check_PROGRAMS = api_test video_dlna_test

api_test_SOURCES = tap.c api_test.c test.c test_background.c
api_test_CFLAGS = $(LMS_INCLUDE)
api_test_LDADD = $(LMS_LTLIB) -lcunit

video_dlna_test_SOURCES = tap.c video_dlna_test.c test.c test_background.c
video_dlna_test_CFLAGS = $(LMS_INCLUDE)
video_dlna_test_LDADD = $(LMS_LTLIB) -lcunit

TESTS = api_test video_dlna_test

include_HEADERS = common.h tap.h
