CPPFLAGS = @CPPFLAGS@
CPPFLAGS += -DCP_HOST="\"$(host)\""

LIBS = @LIBS@
LIBS += $(LIBS_OTHER)

check_PROGRAMS = testsuite

testsuite_SOURCES = loggers.c collections.c initdestroy.c fatalerror.c cpinfo.c testmain.c test.h
testsuite_LDFLAGS = -dlopen self

check-local:
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
