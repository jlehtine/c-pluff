## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

LIBS = @LIBS@
LIBS += $(LIBS_OTHER)

plugindir = /plugins/callbackcounter

plugin_LTLIBRARIES = libruntime.la
plugin_DATA = plugin.xml

libruntime_la_SOURCES = callbackcounter.c callbackcounter.h
libruntime_la_LDFLAGS = -module -avoid-version
