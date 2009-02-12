#!/bin/sh

echo "running aclocal"
aclocal || exit 1
echo "running libtoolize"
libtoolize --force || exit 1
echo "running automake"
automake -a -c || exit 1
echo "running autoconf"
autoconf || exit 1
echo "running configure"
if test x$NOCONFIGURE = x; then
	./configure "$@" || exit 1
fi
