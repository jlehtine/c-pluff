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
libcpluff_la_SOURCES = ploader.c pcontrol.c core.c util.c kazlib/list.c kazlib/hash.c
libcpluff_la_CPPFLAGS = -DCP_BUILD -DCP_DATADIR="\"$(datadir)\""
libcpluff_la_LDFLAGS = -version-info $(APIVERSION)

include_HEADERS = cpluff.h
noinst_HEADERS = core.h pcontrol.h util.h

noinst_PROGRAMS = testmain
testmain_SOURCES = testmain.c
testmain_LDADD = libcpluff.la
