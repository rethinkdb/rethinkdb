#!/bin/sh
P=`pwd`/libatomic_ops-install
cd libatomic_ops-*[0-9]
./configure --prefix=$P
