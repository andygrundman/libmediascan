AM_CFLAGS = $(DCADEC_CFLAGS)

noinst_PROGRAMS = compare
compare_SOURCES = compare.c
compare_LDADD = $(COMPARE_LIBS)

dist_check_SCRIPTS = regression compile globals
EXTRA_DIST = tests
TESTS = regression compile
#broken test: globals
