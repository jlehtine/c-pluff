#! /bin/sh

# Copyright 2007 Johannes Lehtinen
# This shell script is free software; Johannes Lehtinen gives
# unlimited permission to copy, distribute and modify it.

set -e

# Check directory
basedir="`dirname "$0"`"
if ! test -f "$basedir"/libcpluff/cpluff.h; then
    echo 'Run autogen.sh in the top level source directory.' 1>&2
    exit 1
fi

# Generate files in top level directory
cd "$basedir"
test -d auxliary || mkdir auxliary
test -d m4 || mkdir m4
test -e po/Makefile.in.in || gettextize
if ! test -e auxliary/config.rpath; then
    d="`type -p gettextize`"
    d="`dirname "$d"`"
    d="`dirname "$d"`"
    ln -s "$d"/share/gettext/config.rpath auxliary/config.rpath
fi
test -e auxliary/ltmain.sh || libtoolize
aclocal -I m4
autoconf
autoheader
automake -a
