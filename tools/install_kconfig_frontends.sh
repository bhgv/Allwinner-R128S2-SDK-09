#!/bin/sh

cd kconfig-frontends
./configure LDFLAGS=-static --disable-shared --enable-static --disable-gconf --disable-qconf --disable-nconf
make LDFLAGS="-all-static -static-libtool-libs"
sudo make install
