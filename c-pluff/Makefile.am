## Process this file with automake to produce Makefile.in.

# Copyright 2006 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libcpluff libcpluffxx loader console examples po
DOC_DIRS = libcpluff libcpluffxx

RECURSIVE_DOC_TARGETS = refdoc impldoc

check-copyright: COPYRIGHT_HOLDER = Johannes Lehtinen
check-copyright:
	( echo; \
		echo "Checking copyright notices for holder $(COPYRIGHT_HOLDER)."; \
		echo "The following source files might have outdated/incomplete copyright notices"; \
		echo "(if $(COPYRIGHT_HOLDER) is a copyright holder for them):"; \
		find $(top_srcdir) -type f \
			\( -name '*.c' -or -name '*.h' -or -name '*.am' -or \
				-name 'configure.ac' -or -name '*.h.in' -or -name '*.xsd' \
				-or -name '*.pot' -or -name doxygen.cfg.in \) \
			! -exec sh -c "grep -q -E \"Copyright (\`date -r '{}' '+%Y'\`|`date '+%Y'`) $(COPYRIGHT_HOLDER)\" '{}'" \; \
			-print )

$(RECURSIVE_DOC_TARGETS):
	@target=$@; \
	for d in $(DOC_DIRS); do \
		(cd $$d && $(MAKE) $(AM_MAKEFLAGS) $$target) || eval exit 1; \
	done

.PHONY: check-copyright $(RECURSIVE_DOC_TARGETS)
