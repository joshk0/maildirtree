#!/bin/sh

autoheader
autoconf

cp -f /usr/share/automake/install-sh .

sh configure $*
