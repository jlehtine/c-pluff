## Process this file with automake to produce Makefile.in.

# Copyright 2006 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

CP_API_VERSION = $(CP_API_CURRENT):$(CP_API_REVISION):$(CP_API_AGE)

lib_LTLIBRARIES = libcpluff.la
libcpluff_la_SOURCES = ploader.c pcontrol.c context.c cpluff.c util.c ../kazlib/list.c ../kazlib/hash.c
if THREADS_POSIX
libcpluff_la_SOURCES += thread_posix.c
endif
if THREADS_WINDOWS
libcpluff_la_SOURCES += thread_windows.c
endif
libcpluff_la_CPPFLAGS = -DCP_BUILD -DCP_HOST="\"$(host)\"" -DCP_DATADIR="\"$(datadir)\""
libcpluff_la_LDFLAGS = -version-info $(CP_API_VERSION) -no-undefined

include_HEADERS = cpluff.h
noinst_HEADERS = defines.h context.h pcontrol.h util.h
