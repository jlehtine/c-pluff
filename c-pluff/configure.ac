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

# C/C++ compiler settings
# -------------------
AC_PROG_CC
if test -z "$CC"; then
  AC_MSG_ERROR([C compiler was not found (required)])
fi
AC_LANG([C])
AC_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_CXXCPP
AC_C_CONST
AC_PROG_RANLIB
AC_DEFINE([CP_BUILD])


# Thread support checks
# ---------------------

# Check the options
AC_ARG_ENABLE([threads], AS_HELP_STRING([--enable-threads@<:@=TYPE@:>@], [enable multi-threading support (supported TYPEs are "Posix" and "wxWidgets")]))
case "$enable_threads" in
  yes)
  	# We try to detect them by default
  	enable_threads=''
  	;;
  ''|no|Posix|wxWidgets)
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
    AC_CHECK_HEADER([pthread.h], [cp_threads=Posix])
  fi
  
  # Check for wxWidget thread support
  if test -z "$cp_threads" && \
    ( test -z "$enable_threads" || test "$enable_threads" = wxWidgets ) && \
    test "$cross_compiling" != yes; then
    if test -z "$CXX"; then
      AC_MSG_WARN([not considering wxWidgets threads because C++ compiler not found])
    else
      AC_CHECK_PROGS([WXCONFIG], [wx-config])
      if test -n "$WXCONFIG"; then
        AC_LANG([C++])
        old_CPPFLAGS="$CPPFLAGS"
        CPPFLAGS="$CPPFLAGS `"$WXCONFIG" --cppflags`"
        AC_CHECK_HEADER([wx/thread.h], [cp_threads=wxWidgets], [CPPFLAGS="$old_CPPFLAGS"])
        AC_LANG([C])
      fi
    fi
  fi
  
  # Check if we got the desired thread support
  if test -n "$enable_threads" && test "$enable_threads" != "$cp_threads"; then
    AC_MSG_ERROR([$enable_threads threads not detected])
  fi
  
fi

# Inform the user about some choices
# ----------------------------------

if test -z "$cp_threads"; then
  if test "$enable_threads" != no; then
    AC_MSG_WARN([multi-threading support not detected])
  fi
else
  AC_MSG_NOTICE([compiling with $cp_threads thread support])
  case "$cp_threads" in
    Posix)
      AC_DEFINE([CP_THREADS_POSIX])
      ;;
    wxWidgets)
      AC_DEFINE([CP_THREADS_WXWIDGETS])
      ;;
  esac
fi

# Output Makefiles
# ----------------

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
