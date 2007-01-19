## Process this file with automake to produce Makefile.in.

# Copyright 2006 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

LIBS = @LIBS@
LIBS += $(LIBS_OTHER)

bin_PROGRAMS = cpluff-console

cpluff_console_SOURCES = console.c console.h
if HAVE_READLINE
cpluff_console_SOURCES += cmdinput_readline.c
else
cpluff_console_SOURCES += cmdinput_basic.c
endif
cpluff_console_CPPFLAGS = -DCP_DATADIR="\"$(datadir)\"" -DCP_PKGDATADIR="\"$(pkgdatadir)\""
