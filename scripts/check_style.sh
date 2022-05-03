#!/bin/bash
# Copyright 2010-2022 RethinkDB, all rights reserved.

# Check the style of C++ code using cpplint
#
# - With no arguments, checks all files in `src/'
# - Otherwise, check only the files mentioned on the command line

set -euo pipefail

DIR=$(dirname "$0")
SRC_DIR=$(dirname "$DIR")/src
# Get the number of CPU cores for xargs parallel execution
NUM_CORES=$(python -c 'import multiprocessing; print(multiprocessing.cpu_count())')
ARGC=$#

# Return those files that will be tested with cpplint.
# If no arguments were provided for the script, all files are checked,
# otherwise, only those that are requested in the script arguments.
function files_to_check() {
  if [ $ARGC = 0 ]; then
    find "${SRC_DIR}" -name \*.h -o -name \*.hpp -o -name \*.cc -o -name \*.tcc
  else
    echo "$@"
  fi
}

# Format the output depending on the script arguments.
# If no arguments were provided for the script, all ignore-related output is
# hidden, otherwise, show ignored files if the style check was requested for
# specific files.
function format_output() {
  if [ $ARGC = 0 ]; then
    IGNORE_EXCLUDED='/^Ignoring.*file excluded/d'
  else
    IGNORE_EXCLUDED=
  fi
  sed '/^Total errors found:/d;'"${IGNORE_EXCLUDED}"
}

function main() {
  files_to_check "$@" | xargs \
    -n 20 \
    -P "${NUM_CORES}" \
    "${DIR}/cpplint" \
    --verbose=2 \
    --root="${SRC_DIR}" \
    --extensions=h,hpp,cc,tcc 2>&1 | format_output
}

main "$@"
