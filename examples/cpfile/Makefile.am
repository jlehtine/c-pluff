## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

SUBDIRS = plugins

EXTRA_DIST = README.txt cpfile.in

all-local: cpfile

cpfile: $(srcdir)/cpfile.in
	sed -e 's:@''bindir@:$(bindir):g' -e 's:@''exampleslibdir@:$(exampleslibdir):g' < $< > $@

install-exec-local:
	mkdir -p $(DESTDIR)$(examplesbindir)
	cp cpfile $(DESTDIR)$(examplesbindir)
	chmod 755 $(DESTDIR)$(examplesbindir)/cpfile
