#! /bin/sh

set -e

test -e auxliary/ltmain.sh || libtoolize
aclocal
automake -a
autoconf
