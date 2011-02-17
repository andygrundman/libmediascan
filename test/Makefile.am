LMS_INCLUDE = -I$(top_srcdir)/include
LMS_LTLIB = ../src/libmediascan.la

check_PROGRAMS = api_test
api_test_SOURCES = tap.c api_test.c
api_test_CFLAGS = $(LMS_INCLUDE)
api_test_LDADD = $(LMS_LTLIB)

TESTS = api_test
