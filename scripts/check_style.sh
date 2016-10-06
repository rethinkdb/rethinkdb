#!/bin/bash
# Copyright 2010-2016 RethinkDB, all rights reserved.

# Check the style of C++ code using cpplint
#
# - With no arguments, checks all files in `src/'
# - Otherwise, check only the files mentioned on the command line

set -eu
set -o pipefail

DIR=`dirname $0`
SRC_DIR=`dirname "$DIR"`/src
ARG_COUNT=$#
XARGS_FLAGS="-n 20 -P `python -c 'import multiprocessing; print(multiprocessing.cpu_count())'`" # Run multiple cpplint in parallel

all_files () {
    find "$SRC_DIR" -name \*.h -o -name \*.hpp -o -name \*.cc -o -name \*.tcc
}

files_to_check () {
    if [ $ARG_COUNT = 0 ]; then
        all_files
    else
        printf "%s\n" "$@"
    fi
}

format_output () {
    if [ $ARG_COUNT = 0 ]; then
        IGNORE_EXCLUDED='/^Ignoring.*file excluded/d'
    else
        IGNORE_EXCLUDED=
    fi
    sed '/^Total errors found:/d;'"$IGNORE_EXCLUDED"
}

files_to_check "$@" | xargs $XARGS_FLAGS "$DIR"/cpplint --verbose=2 --root="$SRC_DIR" --extensions=h,hpp,cc,tcc 2>&1 | format_output
