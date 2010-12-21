LMS_INCLUDE = -I$(top_srcdir)/include
LMS_LTLIB = ../src/libmediascan.la

check_PROGRAMS = scan_test
scan_test_SOURCES = tap.c scan_test.c
scan_test_CFLAGS = $(LMS_INCLUDE)
scan_test_LDADD = $(LMS_LTLIB)

TESTS = scan_test