LIBS = @LIBS@
LIBS += $(LIBS_OTHER)

plugindir = /plugins/callbackcounter

plugin_LTLIBRARIES = libruntime.la
plugin_DATA = plugin.xml

libruntime_la_SOURCES = callbackcounter.c callbackcounter.h
libruntime_la_LDFLAGS = -module -avoid-version
