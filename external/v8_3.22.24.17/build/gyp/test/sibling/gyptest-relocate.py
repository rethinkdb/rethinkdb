#!/usr/bin/env python

# Copyright (c) 2012 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('build/all.gyp', chdir='src')

test.relocate('src', 'relocate/src')

test.build('build/all.gyp', test.ALL, chdir='relocate/src')

chdir = 'relocate/src/build'

# The top-level Makefile is in the directory where gyp was run.
# TODO(mmoss) Should the Makefile go in the directory of the passed in .gyp
# file? What about when passing in multiple .gyp files? Would sub-project
# Makefiles (see http://codereview.chromium.org/340008 comments) solve this?
if test.format in ('make', 'ninja'):
  chdir = 'relocate/src'

if test.format == 'xcode':
  chdir = 'relocate/src/prog1'
test.run_built_executable('program1',
                          chdir=chdir,
                          stdout="Hello from prog1.c\n")

if test.format == 'xcode':
  chdir = 'relocate/src/prog2'
test.run_built_executable('program2',
                          chdir=chdir,
                          stdout="Hello from prog2.c\n")

test.pass_test()
