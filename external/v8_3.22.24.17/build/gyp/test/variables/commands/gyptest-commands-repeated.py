#!/usr/bin/env python

# Copyright (c) 2012 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Test variable expansion of '<!()' syntax commands where they are evaluated
more then once..
"""

import TestGyp

test = TestGyp.TestGyp(format='gypd')

expect = test.read('commands-repeated.gyp.stdout').replace('\r\n', '\n')

test.run_gyp('commands-repeated.gyp',
             '--debug', 'variables',
             stdout=expect, ignore_line_numbers=True)

# Verify the commands-repeated.gypd against the checked-in expected contents.
#
# Normally, we should canonicalize line endings in the expected
# contents file setting the Subversion svn:eol-style to native,
# but that would still fail if multiple systems are sharing a single
# workspace on a network-mounted file system.  Consequently, we
# massage the Windows line endings ('\r\n') in the output to the
# checked-in UNIX endings ('\n').

contents = test.read('commands-repeated.gypd').replace('\r\n', '\n')
expect = test.read('commands-repeated.gypd.golden').replace('\r\n', '\n')
if not test.match(contents, expect):
  print "Unexpected contents of `commands-repeated.gypd'"
  test.diff(expect, contents, 'commands-repeated.gypd ')
  test.fail_test()

test.pass_test()
