dnl --------------------------------------------------------------------------
dnl C-Pluff, a plug-in framework for C
dnl Copyright 2005 Johannes Lehtinen
dnl --------------------------------------------------------------------------
dnl
dnl Process this file with autoconf to produce a configure script.

# Autoconf initialization
# -----------------------
AC_INIT([C-Pluff], [0.1], [johannes.lehtinen@iki.fi], [c-pluff])
AC_COPYRIGHT([Copyright 2005 Johannes Lehtinen
This configure script is free software; Johannes Lehtinen gives unlimited
permission to copy, distribute and modify it.])
AC_CONFIG_SRCDIR([cpluff.h])
AC_CONFIG_AUX_DIR([auxliary])

# Automake initialization
# -----------------------
AM_INIT_AUTOMAKE([foreign])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.14.4])
AM_ICONV

# C/C++ compiler settings
# -------------------
AC_LANG([C])
AC_PROG_CC
if test -z "$CC"; then
  AC_MSG_ERROR([C compiler was not found (required)])
fi
AC_PROG_CC_C_O
AC_C_CONST
AC_PROG_RANLIB
AC_PROG_LIBTOOL
CPPFLAGS="$CPPFLAGS -I."

# Thread support checks
# ---------------------

# Check the options
AC_ARG_ENABLE([threads], AS_HELP_STRING([--enable-threads@<:@=TYPE@:>@], [enable multi-threading support (supported TYPEs are "Posix" and "Windows", default is to check support in this order)]))
case "$enable_threads" in
  yes)
  	# Use the default auto detection
  	enable_threads=''
  	;;
  ''|no|Posix|Windows)
  	# Ok, legal value, no action needed
  	;;
  *)
  	AC_MSG_ERROR([$enable_threads threads not supported])
  	;;
esac

# Only check for different thread types if threading not disabled
cp_threads=''
if test "$enable_threads" != no; then

  # Check for Posix thread support
  if test -z "$cp_threads" && \
    ( test -z "$enable_threads" || test "$enable_threads" = Posix ); then
    AC_MSG_CHECKING([for Posix threads])
    AC_LINK_IFELSE(
[#include <pthread.h>

int main(int argc, char *argv[]) {
	pthread_mutex_t mutex;
	
	pthread_mutex_init(&mutex, NULL);
	return 0;
}
], [AC_MSG_RESULT([yes])
cp_threads=Posix], AC_MSG_RESULT([no]))
  fi
  
  # Check for Windows thread suppport
  if test -z "$cp_threads" && \
    ( test -z "$enable_threads" || test "$enable_threads" = Windows ); then
    AC_MSG_CHECKING([for Windows threads])
    AC_LINK_IFELSE(
[#include <windows.h>

int main(int argc, char *argv[]) {
  CreateMutex(NULL, FALSE, NULL);
  return 0;
}
], [AC_MSG_RESULT([yes])
cp_threads=Windows], AC_MSG_RESULT([no]))
  fi
  
  # Check if we got the desired thread support
  if test -n "$enable_threads" && test "$enable_threads" != "$cp_threads"; then
    AC_MSG_ERROR([$enable_threads threads not detected])
  fi
  
fi

if test -z "$cp_threads" && test "$enable_threads" != no; then
  AC_MSG_WARN([multi-threading support not detected])
fi

case "$cp_threads" in
  Posix)
    AC_DEFINE([CP_THREADS_POSIX], [], [Enable Posix thread support])
    ;;
  Windows)
    AC_DEFINE([CP_THREADS_WINDOWS], [], [Enable Windows thread support])
    ;;
esac


# Output Makefiles
# ----------------

AC_CONFIG_FILES([Makefile po/Makefile.in])
AC_OUTPUT


# Print configuration information
# -------------------------------
AC_MSG_NOTICE([-----------------------------------------------------------])
AC_MSG_NOTICE([C-Pluff configuration])
if test -n "$cp_threads"; then
  val="yes ($cp_threads)"
else
  val=no
fi
AC_MSG_NOTICE([  multi-threading support: $val])
AC_MSG_NOTICE([-----------------------------------------------------------])
