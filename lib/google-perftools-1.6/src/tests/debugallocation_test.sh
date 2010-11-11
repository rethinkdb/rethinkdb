#!/bin/sh

# Copyright (c) 2009, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# ---
# Author: Craig Silverstein

BINDIR="${BINDIR:-.}"

if [ "x$1" = "x-h" -o "x$1" = "x--help" ]; then
  echo "USAGE: $0 [unittest dir]"
  echo "       By default, unittest_dir=$BINDIR"
  exit 1
fi

DEBUGALLOCATION_TEST="${1:-$BINDIR/debugallocation_test}"

num_failures=0

# Run the i-th death test and make sure the test has the expected
# regexp.  We can depend on the first line of the output being
#    Expected regex:<regex>
# Evaluates to "done" if we are not actually a death-test (so $1 is
# too big a number, and we can stop).  Evaluates to "" otherwise.
# Increments num_failures if the death test does not succeed.
OneDeathTest() {
  "$DEBUGALLOCATION_TEST" "$1" 2>&1 | {
    read regex_line
    regex=`expr "$regex_line" : "Expected regex:\(.*\)"`
    test -z "$regex" && echo "done"  # no regex line, not a death-case
    grep "$regex" >/dev/null 2>&1    # pass the rest of the lines through grep
  } || num_failures=`expr $num_failures + 1`
}

death_test_num=0   # which death test to run
while test -z `OneDeathTest "$death_test_num"`; do
  echo "Done with death test $death_test_num"
  death_test_num=`expr $death_test_num + 1`
done

# Test the non-death parts of the test too
if ! "$DEBUGALLOCATION_TEST"; then
  num_failures=`expr $num_failures + 1`
fi

if [ "$num_failures" = 0 ]; then
  echo "PASS"
else
  echo "Failed with $num_failures failures"
fi
exit $num_failures
