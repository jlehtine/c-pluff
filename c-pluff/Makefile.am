# --------------------------------------------------------------------------
# C-Pluff, a plug-in framework for C
# Copyright 2005-2006 Johannes Lehtinen
# --------------------------------------------------------------------------
#
# Process this file with automake to produce a real makefile.

APIVERSION = 0:0:0

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = po

lib_LTLIBRARIES = libcpluff.la
libcpluff_la_SOURCES = ploader.c pcontrol.c context.c util.c kazlib/list.c kazlib/hash.c
if THREADS_POSIX
libcpluff_la_SOURCES += thread_posix.c
endif
if THREADS_WINDOWS
libcpluff_la_SOURCES += thread_windows.c
endif
libcpluff_la_CPPFLAGS = -DCP_BUILD -DCP_DATADIR="\"$(datadir)\""
libcpluff_la_LDFLAGS = -version-info $(APIVERSION) -no-undefined

include_HEADERS = cpluff.h
noinst_HEADERS = context.h pcontrol.h util.h

noinst_PROGRAMS = testmain
testmain_SOURCES = testmain.c
testmain_LDADD = libcpluff.la
