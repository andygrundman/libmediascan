LMS_INCLUDE = -I$(top_srcdir)/include -I$(top_srcdir)/test
LMS_LTLIB = ../src/libmediascan.la

check_PROGRAMS = api_test image_test
#video_dlna_test image_test

api_test_SOURCES = tap.c api_test.c test.c test_background.c test_images.c
api_test_CFLAGS = $(LMS_INCLUDE)
api_test_LDADD = $(LMS_LTLIB) -lcunit

#video_dlna_test_SOURCES = tap.c video_dlna_test.c test.c test_background.c
#video_dlna_test_CFLAGS = $(LMS_INCLUDE)
#video_dlna_test_LDADD = $(LMS_LTLIB) -lcunit -ldb

image_test_SOURCES = tap.c image_test.c
image_test_CFLAGS = $(LMS_INCLUDE)
image_test_LDADD = $(LMS_LTLIB) -lcunit -ldb

TESTS = api_test
#video_dlna_test image_test

include_HEADERS = common.h tap.h
