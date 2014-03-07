#!/usr/bin/env python

# Copyright (c) 2011 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Verifies simple rules when using an explicit build target of 'all'.
"""

import TestGyp
import os

test = TestGyp.TestGyp(formats=['make', 'ninja', 'xcode'])

test.run_gyp('actions.gyp', chdir='src')

test.relocate('src', 'relocate/src')

test.build('actions.gyp', chdir='relocate/src')

expect = """\
hi c
hello baz
"""
if test.format == 'xcode':
  chdir = 'relocate/src/subdir'
else:
  chdir = 'relocate/src'
test.run_built_executable('gencc_int_output', chdir=chdir, stdout=expect)

test.must_match('relocate/src/subdir/foo/bar/baz.printed',
                os.path.join('foo', 'bar'))
test.must_match('relocate/src/subdir/a/b/c.printed', os.path.join('a', 'b'))

test.pass_test()
