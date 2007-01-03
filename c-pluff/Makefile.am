## Process this file with automake to produce Makefile.in.

# Copyright 2006 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libcpluff plugins console po

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

.PHONY: check-copyright
