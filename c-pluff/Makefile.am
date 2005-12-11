# --------------------------------------------------------------------------
# C-Pluff, a plug-in framework for C
# Copyright 2005 Johannes Lehtinen
# --------------------------------------------------------------------------
#
# Process this file with automake to produce a real makefile.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = po

AM_CFLAGS = -Wall -Wmissing-prototypes -pedantic
AM_CPPFLAGS = -DCP_BUILD -DCP_DATADIR="\"$(datadir)\""
#AM_CPPFLAGS += -DNDEBUG

lib_LTLIBRARIES = libcpluff.la
libcpluff_la_SOURCES = ploader.c pcontrol.c core.c util.c kazlib/list.c kazlib/hash.c

noinst_PROGRAMS = testmain
testmain_SOURCES = testmain.c
testmain_LDADD = libcpluff.la
