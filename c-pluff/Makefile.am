## Process this file with automake to produce Makefile.in.

# Copyright 2006 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libcpluff plugins console po

DOCPP = doc++
DOCPP_FLAGS =
DOCPP_HTML_FLAGS = -w -W
DOCPP_TEX_FLAGS = -t -X
DOCPP_REFROOT = $(srcdir)/libcpluff/cpluff.h
DOCPP_REFFILES = $(DOCPP_REFROOT) $(srcdir)/libcpluff/cpluffdef.h.in

PDFTEX = pdflatex

check-copyright: COPYRIGHT_HOLDER = Johannes Lehtinen
check-copyright:
	( echo; \
		echo "Checking copyright notices for holder $(COPYRIGHT_HOLDER)."; \
		echo "The following source files might have outdated/incomplete copyright notices"; \
		echo "(if $(COPYRIGHT_HOLDER) is a copyright holder for them):"; \
		find $(top_srcdir) -type f \
			\( -name '*.c' -or -name '*.h' -or -name '*.am' -or \
				-name 'configure.ac' -or -name '*.h.in' -or -name '*.xsd' \
				-or -name '*.pot' \) \
			! -exec sh -c "grep -q -E \"Copyright (\`date -r '{}' '+%Y'\`|`date '+%Y'`) $(COPYRIGHT_HOLDER)\" '{}'" \; \
			-print )

dist-hook: doc-reference
	mkdir -p $(distdir)/doc
	cp -a doc/reference $(distdir)/doc

doc-reference: doc-ref-html
doc-ref-html: doc/reference/html/index.html
doc-ref-pdf: doc/reference/cpluff-ref.pdf

doc/reference/html/index.html: $(DOCPP_REFFILES)
	rm -rf doc/reference/html
	mkdir -p doc/reference/html
	$(DOCPP) $(DOCPP_FLAGS) $(DOCPP_HTML_FLAGS) -d doc/reference/html $(DOCPP_REFROOT)

doc/reference/cpluff-ref.pdf: cpluff-ref.tex
	$(PDFTEX) cpluff-ref.tex
	$(PDFTEX) cpluff-ref.tex
	mkdir -p doc/reference
	mv cpluff-ref.pdf doc/reference

cpluff-ref.tex: $(DOCPP_REFFILES)
	$(DOCPP) $(DOCPP_FLAGS) $(DOCPP_TEX_FLAGS) -o $@ $(DOCPP_REFROOT)

.PHONY: check-copyright doc-reference doc-ref-html doc-ref-pdf
