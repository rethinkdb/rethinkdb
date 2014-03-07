#!/bin/sh
# Copyright (c) 2007, International Business Machines Corporation and
# others. All Rights Reserved.

# Echo shell script commands.
set -ex

PERF=test/perf/unisetperf/unisetperf
# slow Bh bh Bv Bv0 B0 BvF Bvp BvpF L Bvl BvL
# --pattern [:White_Space:]

for file in udhr_eng.txt \
            udhr_deu.txt \
            udhr_fra.txt \
            udhr_rus.txt \
            udhr_tha.txt \
            udhr_jpn.txt \
            udhr_cmn.txt \
            udhr_jpn.html; do
  for type in slow BvF BvpF Bvl BvL; do
    $PERF SpanUTF8  --type $type -f ~/udhr/$file -v -e UTF-8 --passes 3 --iterations 10000
  done
done
