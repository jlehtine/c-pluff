# --------------------------------------------------------------------------
# C-Pluff, a plug-in framework for C
# Copyright 2005-2006 Johannes Lehtinen
# --------------------------------------------------------------------------
#
# Process this file with automake to produce a real makefile.

bin_PROGRAMS = cpluff-console
noinst_HEADERS = console.h

cpluff_console_SOURCES = console.c
if HAVE_READLINE
cpluff_console_SOURCES += cmdinput_readline.c
else
cpluff_console_SOURCES += cmdinput_basic.c
endif
cpluff_console_LDADD = ../libcpluff/libcpluff.la
if HAVE_READLINE
cpluff_console_LDADD += -lreadline
endif
cpluff_console_CPPFLAGS = -DCP_DATADIR="\"$(datadir)\""
