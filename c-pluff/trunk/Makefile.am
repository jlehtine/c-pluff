## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libcpluff loader console po test
DIST_SUBDIRS = $(SUBDIRS) doc examples

EXTRA_DIST = COPYRIGHT.txt BUILD.txt ChangeLog autogen.sh doxygen.footer doxygen.css plugin.xsd

doc:
	cd libcpluff && $(MAKE) $(AM_MAKEFLAGS) $@

examples: all
	cd examples && $(MAKE) $(AM_MAKEFLAGS) all

examples-install: install
	cd examples && $(MAKE) $(AM_MAKEFLAGS) install

examples-clean:
	cd examples && $(MAKE) $(AM_MAKEFLAGS) clean

dist-hook:
	rm -f $(distdir)/ABOUT-NLS
	rm -f $(distdir)/doc/*~

clean-local: examples-clean

distclean-local:
	cd examples && $(MAKE) $(AM_MAKEFLAGS) distclean

.PHONY: doc examples examples-install examples-clean
