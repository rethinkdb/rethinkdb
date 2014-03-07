#!/usr/bin/env python

# Copyright (c) 2012 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Verifies that xcode-style GCC_... settings are handled properly.
"""

import TestGyp

import os
import sys

def IgnoreOutput(string, expected_string):
  return True

if sys.platform == 'darwin':
  test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'])

  CHDIR = 'xcode-gcc'
  test.run_gyp('test.gyp', chdir=CHDIR)

  # List of targets that'll pass. It expects targets of the same name with
  # '-fail' appended that'll fail to build.
  targets = [
    'warn_about_missing_newline',
  ]

  # clang doesn't warn on invalid offsetofs, it silently ignores
  # -Wno-invalid-offsetof.
  # TODO(thakis): This isn't really the right way to detect the compiler,
  # `which cc` detects what make ends up using, and Xcode has some embedded
  # compiler, but it's a reliable proxy at least on the bots.
  if os.readlink('/usr/bin/cc') != 'clang':
    targets.append('warn_about_invalid_offsetof_macro')

  for target in targets:
    test.build('test.gyp', target, chdir=CHDIR)
    test.built_file_must_exist(target, chdir=CHDIR)
    fail_target = target + '-fail'
    test.build('test.gyp', fail_target, chdir=CHDIR, status=None,
               stderr=None, match=IgnoreOutput)
    test.built_file_must_not_exist(fail_target, chdir=CHDIR)

  test.pass_test()
