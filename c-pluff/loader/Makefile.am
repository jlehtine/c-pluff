## Process this file with automake to produce Makefile.in.

LIBS = @LIBS@
LIBS += $(LIBS_OTHER)

bin_PROGRAMS = cpluff-loader

cpluff_loader_SOURCES = loader.c
cpluff_loader_CPPFLAGS = -DCP_DATADIR="\"$(datadir)\""
