## Process this file with automake to produce Makefile.in.

# Copyright 2007 Johannes Lehtinen
# This Makefile is free software; Johannes Lehtinen gives unlimited
# permission to copy, distribute and modify it.

LIBS = @LIBS_OTHER@ @LIBS@

pluginsdir = $(exampleslibdir)/cpfile/plugins
plugindir = $(pluginsdir)/core

plugin_LTLIBRARIES = libcore.la
plugin_DATA = plugin.xml classifiers.xsd
plugin_HEADERS = core.h

EXTRA_DIST = plugin.xml classifiers.xsd

libcore_la_SOURCES = core.c core.h
libcore_la_LDFLAGS = -module -avoid-version
