#!/bin/sh
GC_DEBUG=1
export GC_DEBUG
$* 2>&1 | awk '{print "0x3e=c\""$0"\""};/^\t##PC##=/ {if ($2 != 0) {print $2"?i"}}' | adb $1 | sed "s/^		>/>/"
