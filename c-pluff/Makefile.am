# --------------------------------------------------------------------------
# C-Pluff, a plug-in framework for C
# Copyright 2005 Johannes Lehtinen
# --------------------------------------------------------------------------
#
# Process this file with automake to produce a real makefile.

AM_CFLAGS = -Wall -Wmissing-prototypes -pedantic
#AM_CPPFLAGS = -DNDEBUG

lib_LIBRARIES = libcpluff.a
libcpluff_a_SOURCES = plugin.c core.c util.c kazlib/list.c kazlib/hash.c

noinst_PROGRAMS = testmain
testmain_SOURCES = testmain.c
testmain_LDADD = libcpluff.a
