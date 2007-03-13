## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

LIBS = @LIBS@
LIBS += $(LIBS_LIBCPLUFF)

EXTRA_DIST = refdoc.dox

CPPFLAGS = @CPPFLAGS@
CPPFLAGS += -I. -DCP_C_API=CP_EXPORT -DCP_HOST="\"$(host)\"" -DCP_DATADIR="\"$(datadir)\""

DOXYGEN = doxygen
DOXYGEN_SOURCE = cpluffdef.h $(srcdir)/cpluff.h $(srcdir)/refdoc.dox
DOXYGEN_STYLE = $(top_srcdir)/doxygen.footer $(top_srcdir)/doxygen.css

lib_LTLIBRARIES = libcpluff.la
libcpluff_la_SOURCES = psymbol.c pscan.c ploader.c pinfo.c pcontrol.c serial.c logging.c context.c cpluff.c util.c ../kazlib/list.c ../kazlib/list.h ../kazlib/hash.c ../kazlib/hash.h internal.h thread.h util.h defines.h
if POSIX_THREADS
libcpluff_la_SOURCES += thread_posix.c
endif
if WINDOWS_THREADS
libcpluff_la_SOURCES += thread_windows.c
endif
libcpluff_la_LDFLAGS = -no-undefined -version-info $(CP_C_LIB_VERSION)

include_HEADERS = cpluff.h cpluffdef.h

refdoc: $(top_srcdir)/doc/reference/c-api/index.html

$(top_srcdir)/doc/reference/c-api/index.html: $(DOXYGEN_SOURCE) Doxyfile-ref $(DOXYGEN_STYLE)
	mkdir -p doxygen-ref
	cp -p $^ doxygen-ref
	cd doxygen-ref && $(DOXYGEN) Doxyfile-ref
	mkdir -p $(top_srcdir)/doc/reference
	rm -rf $(top_srcdir)/doc/reference/c-api
	mv doxygen-ref/html $(top_srcdir)/doc/reference/c-api
	rm -rf doxygen-ref

impldoc: $(top_srcdir)/doc/implementation/c-api/index.html

$(top_srcdir)/doc/implementation/c-api/index.html: $(srcdir)/*.h $(srcdir)/*.c cpluffdef.h ../config.h Doxyfile-impl $(DOXYGEN_STYLE)
	mkdir -p doxygen-impl
	cp -p $^ doxygen-impl
	cd doxygen-impl && $(DOXYGEN) Doxyfile-impl
	mkdir -p $(top_srcdir)/doc/implementation
	rm -rf $(top_srcdir)/doc/implementation/c-api
	mv doxygen-impl/html $(top_srcdir)/doc/implementation/c-api
	rm -rf doxygen-impl

.PHONY: refdoc impldoc
