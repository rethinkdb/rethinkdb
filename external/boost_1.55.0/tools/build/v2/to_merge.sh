#!/bin/bash

for python in build/*.py; do
    jam="${python%.py}.jam"
    if [ -f "$jam" ]; then
        line=`grep "Base revision" $python`
        revision=`echo $line | sed 's/# Base revision: \([0-9]*\).*/\1/'`
        if [ -e "$revision" ] ; then
            echo "No base version for $python" >&2
        else
            svn diff -r $revision:HEAD "$jam"
        fi
    fi
done


