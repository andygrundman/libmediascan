SUBDIRS = include src test

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = bindings test/data

# Remove Perl build files if they exist
dist-hook:
	rm -rf $(distdir)/bindings/perl/Makefile.old \
		$(distdir)/bindings/perl/Makefile \
		$(distdir)/bindings/perl/Scan.o \
		$(distdir)/bindings/perl/Scan.c \
		$(distdir)/bindings/perl/*.bs \
		$(distdir)/bindings/perl/*blib \
		$(distdir)/bindings/perl/MANIFEST.SKIP
