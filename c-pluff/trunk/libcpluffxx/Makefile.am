## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

LIBS = @LIBS_OTHER@ @LTLIBINTL@ @LIBS@

CPPFLAGS = @CPPFLAGS@
CPPFLAGS += -DCP_CXX_API=CP_EXPORT

DOXYGEN = doxygen
DOXYGEN_SOURCE = ../libcpluff/cpluffdef.h $(srcdir)/cpluffxx.h
DOXYGEN_STYLE = $(top_srcdir)/docsrc/doxygen.footer $(top_srcdir)/docsrc/doxygen.css

lib_LTLIBRARIES = libcpluffxx.la
libcpluffxx_la_SOURCES = CPPluginContainerImpl.cc \
	CPPluginContextImpl.cc CPFramework.cc CPFrameworkImpl.cc \
	CPReferenceCountedImpl.cc CPAPIErrorImpl.cc internal.h
libcpluffxx_la_LDFLAGS = -no-undefined -version-info $(CP_CXX_LIB_VERSION)

include_HEADERS = cpluffxx.h

doc: refdoc

refdoc: doc/reference/cxx-api/index.html

doc/reference/cxx-api/index.html: $(DOXYGEN_SOURCE) docsrc/Doxyfile-ref $(DOXYGEN_STYLE)
	rm -rf doxygen-ref
	mkdir -p doxygen-ref
	cp -p $^ doxygen-ref
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
