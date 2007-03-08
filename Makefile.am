## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libcpluff loader console po test

EXTRA_DIST = COPYRIGHT.txt autogen.sh doc doxygen.footer doxygen.css plugin.xsd

doc: refdoc
	cd doc && $(MAKE) $(AM_MAKEFLAGS) $@

refdoc impldoc:
	cd libcpluff && $(MAKE) $(AM_MAKEFLAGS) $@

dist-hook:
	rm -f $(distdir)/ABOUT-NLS $(distdir)/ChangeLog
	rm -f $(distdir)/doc/*~

.PHONY: doc refdoc impldoc
