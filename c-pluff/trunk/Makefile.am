## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libcpluff loader console po test

EXTRA_DIST = COPYRIGHT.txt autogen.sh doc doxygen.footer doxygen.css plugin.xsd

DOC_DIRS = libcpluff
RECURSIVE_DOC_TARGETS = refdoc impldoc

$(RECURSIVE_DOC_TARGETS):
	@target=$@; \
	for d in $(DOC_DIRS); do \
		(cd $$d && $(MAKE) $(AM_MAKEFLAGS) $$target) || eval exit 1; \
	done

dist-hook:
	rm -f $(distdir)/ABOUT-NLS $(distdir)/ChangeLog

.PHONY: $(RECURSIVE_DOC_TARGETS)
