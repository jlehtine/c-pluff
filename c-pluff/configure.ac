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

# C compiler settings
# -------------------
AC_PROG_CC
AC_PROG_CC_C_O
AC_C_CONST
AC_PROG_RANLIB

# Check for Posix thread support
AC_CHECK_HEADER([pthread.h], AC_DEFINE([HAVE_PTHREAD_H]), AC_MSG_WARN([multi-threading support is disabled]))

# Output Makefiles
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
