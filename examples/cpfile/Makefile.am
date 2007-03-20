## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

SUBDIRS = plugins

EXTRA_DIST = README.txt cpfile.in

all-local: cpfile

cpfile: $(srcdir)/cpfile.in
	sed -e 's:@''bindir@:$(bindir):g' -e 's:@''libdir@:$(libdir):g' < $< > $@

install-exec-local:
	cp cpfile $(DESTDIR)$(bindir)
	chmod 755 $(DESTDIR)$(bindir)/cpfile
