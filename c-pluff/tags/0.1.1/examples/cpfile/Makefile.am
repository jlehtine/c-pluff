## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

SUBDIRS = plugins

EXTRA_DIST = README.txt cpfile.in

cpfile: $(srcdir)/cpfile.in
	sed -e 's:@''bindir@:$(bindir):g' -e 's:@''libdir@:$(libdir):g' < $< > $@

install-exec-local: cpfile
	cp cpfile $(bindir)
	chmod 755 $(bindir)/cpfile
