## Process this file with automake to produce Makefile.in.

# Copyright 2006 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

LIBS = @LIBS@
LIBS += $(LIBS_LIBCPLUFF)

CP_CORE_API_VERSION = $(CP_CORE_API_CURRENT):$(CP_CORE_API_REVISION):$(CP_CORE_API_AGE)

DOXYGEN = doxygen
DOXYGEN_SOURCE = cpluffdef.h $(srcdir)/cpluff.h

lib_LTLIBRARIES = libcpluff.la
libcpluff_la_SOURCES = psymbol.c pscan.c ploader.c pinfo.c pcontrol.c serial.c context.c cpluff.c util.c ../kazlib/list.c ../kazlib/hash.c internal.h util.h defines.h
if POSIX_THREADS
libcpluff_la_SOURCES += thread_posix.c
endif
if WINDOWS_THREADS
libcpluff_la_SOURCES += thread_windows.c
endif
libcpluff_la_CPPFLAGS = -I. -DCP_BUILD -DCP_HOST="\"$(host)\"" -DCP_DATADIR="\"$(datadir)\""
libcpluff_la_LDFLAGS = -version-info $(CP_CORE_API_VERSION) -no-undefined

include_HEADERS = cpluff.h cpluffdef.h

refdoc: $(top_srcdir)/doc/reference/core-c/index.html

$(top_srcdir)/doc/reference/core-c/index.html: $(DOXYGEN_SOURCE) Doxyfile-ref
	mkdir -p doxygen-ref
	cp -p $^ doxygen-ref
	cd doxygen-ref && doxygen Doxyfile-ref
	mkdir -p $(top_srcdir)/doc/reference
	rm -rf $(top_srcdir)/doc/reference/core-c
	mv doxygen-ref/html $(top_srcdir)/doc/reference/core-c
	rm -rf doxygen-ref

impldoc: $(top_srcdir)/doc/implementation/core-c/index.html

$(top_srcdir)/doc/implementation/core-c/index.html: $(srcdir)/*.h $(srcdir)/*.c cpluffdef.h config.h Doxyfile-impl
	mkdir -p doxygen-impl
	cp -p $^ doxygen-impl
	cd doxygen-impl && doxygen Doxyfile-impl
	mkdir -p $(top_srcdir)/doc/implementation
	rm -rf $(top_srcdir)/doc/implementation/core-c
	mv doxygen-impl/html $(top_srcdir)/doc/implementation/core-c
	rm -rf doxygen-impl

.PHONY: refdoc impldoc
