CPPFLAGS = @CPPFLAGS@
CPPFLAGS += -DCP_HOST="\"$(host)\""

LIBS = @LIBS@
LIBS += $(LIBS_OTHER)

check_PROGRAMS = testsuite

testsuite_SOURCES = initdestroy.c cpinfo.c testmain.c test.h
testsuite_LDFLAGS = -dlopen self

check-local:
	@numf=0; numt=0; \
	echo; \
	echo '===================================================================='; \
	echo 'C-Pluff Test Suite'; \
	echo '===================================================================='; \
	while read test; do \
		if srcdir='$(srcdir)' ./testsuite $$test; then \
			echo "OK: $$test"; \
		else \
			echo "FAIL: $$test"; \
			numf=$$(($$numf + 1)); \
		fi; \
		numt=$$(($$numt + 1)); \
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
