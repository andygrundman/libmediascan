AM_CFLAGS = $(DCADEC_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = dcadec extract_dca
dcadec_SOURCES = dcadec.c getopt.h getopt.c gettimeofday.h gettimeofday.c
dcadec_LDADD = ../libdca/libdca.la ../libao/libao.la
extract_dca_SOURCES = extract_dca.c getopt.c

man1_MANS = dcadec.1 extract_dca.1
EXTRA_DIST = configure.incl $(man1_MANS)

bin_SCRIPTS = dtsdec extract_dts
MOSTLYCLEANFILES = $(bin_SCRIPTS)

dtsdec extract_dts: Makefile
	{ \
		echo '#! /bin/sh' ; \
		echo 'exec "$$(dirname "$$0")/$$(basename "$$0" | sed -e 's/ts/ca/')" "$$@"' ; \
	} > $@.tmp
	chmod +x $@.tmp
	mv -f $@.tmp $@

install-data-hook:
	for a in dcadec extract_dca; do \
		oldname=`echo "$$a" | sed -e 's/ca/ts/' | sed '$(transform)'` ; \
		name=`echo "$$a" | sed '$(transform)'` ; \
		ln -sf "$(man1dir)/$${name}.1" "$(DESTDIR)$(man1dir)/$${oldname}.1" ; \
	done

uninstall-hook:
	for a in dcadec extract_dca; do \
		oldname=`echo "$$a" | sed -e 's/ca/ts/' | sed '$(transform)'` ; \
		unlink "$(DESTDIR)$(man1dir)/$${oldname}.1" ; \
	done

