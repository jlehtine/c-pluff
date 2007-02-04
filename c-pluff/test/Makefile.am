CPPFLAGS = @CPPFLAGS@
CPPFLAGS += -DCP_HOST="\"$(host)\""

LIBS = @LIBS@
LIBS += $(LIBS_OTHER)

check_PROGRAMS = testsuite

testsuite_SOURCES = pcallbacks.c pscanning.c pinstallation.c loggers.c collections.c initdestroy.c fatalerror.c cpinfo.c testmain.c test.h
testsuite_LDFLAGS = -dlopen self

tmpinstalldir := $(shell mkdir -p tmp-install && cd tmp-install && pwd)

check-local: install-plugins
	@numf=0; numt=0; \
	echo; \
	echo '===================================================================='; \
	echo 'C-Pluff Test Suite'; \
	echo '===================================================================='; \
	while read test; do \
		if test -x "$(srcdir)/test-$$test"; then \
			srcdir='$(srcdir)' "$(srcdir)/test-$$test"; \
			rc=$$?; \
		else \
			srcdir='$(srcdir)' ./testsuite "$$test"; \
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
	done < '$(srcdir)/tests.txt'; \
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

install-plugins: build-plugins install-test-libcpluff
	cd plugins-source && $(MAKE) DESTDIR=$(tmpinstalldir) install

build-plugins:
	cd plugins-source && $(MAKE)

install-test-libcpluff:
	cd ../libcpluff && $(MAKE) DESTDIR=$(tmpinstalldir) install-libLTLIBRARIES

.PHONY: build-plugins install-plugins install-test-libcpluff
