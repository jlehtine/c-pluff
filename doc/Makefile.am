## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

EXTRA_DIST = index.en.html copyright.en.html cpluff_style.css common_style.css img/pageborder.png img/architecture.png

htmldocdir=$(docdir)/html
htmldoc_DATA = index.en.html copyright.en.html cpluff_style.css common_style.css img/pageborder.png img/architecture.png

install-data-local:
	if test -d $(srcdir)/reference; then \
		rm -rf $(DESTDIR)$(htmldocdir)/reference && \
		mkdir -p $(DESTDIR)$(htmldocdir) && \
		cp -r $(srcdir)/reference $(DESTDIR)$(htmldocdir); \
	else \
		cd .. && $(MAKE) $(AM_MAKEFLAGS) doc && \
		rm -rf $(DESTDIR)$(htmldocdir)/reference && \
		mkdir -p $(DESTDIR)$(htmldocdir) && \
		cp -r libcpluff/doc/reference $(DESTDIR)$(htmldocdir) && \
		chmod -R go=rX $(htmldocdir)/reference; \
	fi
