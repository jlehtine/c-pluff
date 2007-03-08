## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

# This can be defined to a debugging wrapper program such as Valgrind.
TEST_WRAPPER =

# This can be defined to a subset of test cases to be performed
SELECTED_TESTS = all

CPPFLAGS = @CPPFLAGS@
CPPFLAGS += -DCP_HOST="\"$(host)\""

LIBS = @LIBS@
LIBS += $(LIBS_OTHER)

check_PROGRAMS = testsuite

testsuite_SOURCES = extcfg.c pdependencies.c pcallbacks.c pscanning.c pinstallation.c ploading.c loggers.c collections.c initdestroy.c fatalerror.c cpinfo.c testmain.c test.h
testsuite_LDFLAGS = -dlopen self

tmpinstalldir := $(shell mkdir -p tmp/install && cd tmp/install && pwd)

check-local: install-plugins
	@numf=0; numt=0; \
	run_test () { \
		local test="$$1"; \
		if test -x "$(srcdir)/test-$$test"; then \
			srcdir='$(srcdir)' TEST_WRAPPER='$(TEST_WRAPPER)' "$(srcdir)/test-$$test"; \
			rc=$$?; \
		else \
			eval srcdir='$(srcdir)' libtool --mode=execute $(TEST_WRAPPER) ./testsuite "\$$test"; \
			rc=$$?; \
		fi; \
		case $$rc in \
			77) \
				echo "SKIPPED: $$test"; \
				;; \
			0) \
				echo "OK: $$test"; \
				numt=$$(($$numt + 1)); \
				;; \
			*) \
				echo "FAIL: $$test"; \
				numt=$$(($$numt + 1)); \
				numf=$$(($$numf + 1)); \
				;; \
		esac; \
	}; \
	echo; \
	echo '===================================================================='; \
	echo 'C-Pluff Test Suite'; \
	echo '===================================================================='; \
	if test '$(SELECTED_TESTS)' = all; then \
		while read test; do \
			run_test "$$test"; \
		done < '$(srcdir)/tests.txt'; \
	else \
		for test in $(SELECTED_TESTS); do \
			run_test "$$test"; \
		done; \
	fi; \
	echo '===================================================================='; \
	if test $$numf -gt 0; then \
		echo "FAILED: $$numf/$$numt"; \
	else \
		echo 'ALL OK!'; \
	fi; \
	echo '===================================================================='; \
	echo; \
	test $$numf -eq 0

mostly-clean-local:
	cd plugins-source && $(MAKE) mostly-clean

clean-local:
	cd plugins-source && $(MAKE) clean

distclean-local:
	cd plugins-source && $(MAKE) distclean

maintainer-clean-local:
	cd plugins-source && $(MAKE) maintainer-clean

install-plugins: build-plugins install-libcpluff
	cd plugins-source && $(MAKE) DESTDIR=$(tmpinstalldir) install

build-plugins:
	cd plugins-source && $(MAKE)

install-libcpluff:
	cd ../libcpluff && $(MAKE) DESTDIR=$(tmpinstalldir) install

.PHONY: build-plugins install-plugins install-libcpluff
