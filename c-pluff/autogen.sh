#! /bin/sh

set -e

test -e po/Makefile.in.in || gettextize
test -e auxliary/ltmain.sh || libtoolize
aclocal -I m4
automake -a
autoconf
