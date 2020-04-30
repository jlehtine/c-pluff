## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

SUBDIRS = cpluffxx

LIBS = @LIBS_OTHER@ @LTLIBINTL@ @LIBS@

CPPFLAGS = @CPPFLAGS@
CPPFLAGS += -DCP_CXX_API=CP_EXPORT

DOXYGEN = doxygen
DOXYGEN_SOURCE_TOP = ../libcpluff/cpluffdef.h $(srcdir)/cpluffxx.h
DOXYGEN_SOURCE_CPLUFFXX = $(srcdir)/cpluffxx/*.h
DOXYGEN_SOURCE = $(DOXYGEN_SOURCE_TOP) $(DOXYGEN_SOURCE_CPLUFFXX)
DOXYGEN_STYLE = $(top_srcdir)/docsrc/doxygen.footer $(top_srcdir)/docsrc/doxygen.css

lib_LTLIBRARIES = libcpluffxx.la
libcpluffxx_la_SOURCES = \
	plugin_container.cc \
	plugin_context.cc framework.cc \
	info.cc util.cc internalxx.h
libcpluffxx_la_LDFLAGS = -no-undefined -version-info $(CP_CXX_LIB_VERSION)

include_HEADERS = cpluffxx.h

pkgconfig_DATA = libcpluffxx.pc

doc: refdoc

refdoc: doc/reference/cxx-api/index.html

doc/reference/cxx-api/index.html: $(DOXYGEN_SOURCE) docsrc/Doxyfile-ref $(DOXYGEN_STYLE)
	rm -rf doxygen-ref
	mkdir -p doxygen-ref
	cp -p $(DOXYGEN_SOURCE_TOP) docsrc/Doxyfile-ref $(DOXYGEN_STYLE) doxygen-ref
	mkdir doxygen-ref/cpluffxx
	cp -p $(DOXYGEN_SOURCE_CPLUFFXX) doxygen-ref/cpluffxx
	cd doxygen-ref && $(DOXYGEN) Doxyfile-ref
	mkdir -p doc/reference
	rm -rf doc/reference/cxx-api
	mv doxygen-ref/html doc/reference/cxx-api
	rm -rf doxygen-ref

dist-hook: refdoc
	mkdir -p $(top_distdir)/doc/reference
	cp -rp doc/reference/cxx-api $(top_distdir)/doc/reference

clean-local:
	rm -rf doc

.PHONY: doc refdoc
