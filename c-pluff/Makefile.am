# --------------------------------------------------------------------------
# C-Pluff, a plug-in framework for C
# Copyright 2005-2006 Johannes Lehtinen
# --------------------------------------------------------------------------
#
# Process this file with automake to produce a real makefile.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libcpluff po

noinst_PROGRAMS = testploader testpcontrol

testploader_SOURCES = testploader.c
testploader_LDADD = libcpluff/libcpluff.la

testpcontrol_SOURCES = testpcontrol.c
testpcontrol_LDADD = libcpluff/libcpluff.la -lreadline
