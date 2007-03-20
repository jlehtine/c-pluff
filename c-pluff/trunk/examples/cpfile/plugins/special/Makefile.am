## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

LIBS = @LIBS@
LIBS += $(LIBS_OTHER)

CPPFLAGS = -I$(srcdir)/../core @CPPFLAGS@

pluginsdir = $(exampleslibdir)/cpfile/plugins
plugindir = $(pluginsdir)/special

plugin_LTLIBRARIES = libspecial.la
plugin_DATA = plugin.xml

EXTRA_DIST = plugin.xml

libspecial_la_SOURCES = special.c
libspecial_la_LDFLAGS = -module -avoid-version
