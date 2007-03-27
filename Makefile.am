## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libcpluff loader console po test po doc
DIST_SUBDIRS = $(SUBDIRS) examples

EXTRA_DIST = COPYRIGHT.txt INSTALL.txt ChangeLog.txt Roadmap.txt autogen.sh doxygen.footer doxygen.css plugin.xsd

doc_DATA = COPYRIGHT.txt ChangeLog.txt Roadmap.txt

doc:
	cd libcpluff && $(MAKE) $(AM_MAKEFLAGS) $@

examples: all
	cd examples && $(MAKE) $(AM_MAKEFLAGS) all LIBS='$(CURDIR)/libcpluff/libcpluff.la $(LIBS)'

examples-install: install
	cd examples && $(MAKE) $(AM_MAKEFLAGS) install

dist-hook:
	rm -f '$(distdir)/ABOUT-NLS'

clean-local:
	test ! -f examples/Makefile || (cd examples && $(MAKE) $(AM_MAKEFLAGS) clean)

.PHONY: doc examples examples-install examples-clean
