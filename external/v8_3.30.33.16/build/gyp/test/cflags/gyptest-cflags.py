#!/usr/bin/env python

# Copyright (c) 2012 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Verifies build of an executable with C++ define specified by a gyp define, and
the use of the environment during regeneration when the gyp file changes.
"""

import os
import sys
import TestGyp

env_stack = []


def PushEnv():
  env_copy = os.environ.copy()
  env_stack.append(env_copy)

def PopEnv():
  os.environ.clear()
  os.environ.update(env_stack.pop())

formats = ['make', 'ninja']

test = TestGyp.TestGyp(formats=formats)

try:
  PushEnv()
  os.environ['CFLAGS'] = ''
  os.environ['GYP_CROSSCOMPILE'] = '1'
  test.run_gyp('cflags.gyp')
  test.build('cflags.gyp')
finally:
  # We clear the environ after calling gyp.  When the auto-regeneration happens,
  # the same define should be reused anyway.  Reset to empty string first in
  # case the platform doesn't support unsetenv.
  PopEnv()


expect = """FOO not defined\n"""
test.run_built_executable('cflags', stdout=expect)
test.run_built_executable('cflags_host', stdout=expect)

test.sleep()

try:
  PushEnv()
  os.environ['CFLAGS'] = '-DFOO=1'
  os.environ['GYP_CROSSCOMPILE'] = '1'
  test.run_gyp('cflags.gyp')
  test.build('cflags.gyp')
finally:
  # We clear the environ after calling gyp.  When the auto-regeneration happens,
  # the same define should be reused anyway.  Reset to empty string first in
  # case the platform doesn't support unsetenv.
  PopEnv()


expect = """FOO defined\n"""
test.run_built_executable('cflags', stdout=expect)

# Environment variables shouldn't influence the flags for the host.
expect = """FOO not defined\n"""
test.run_built_executable('cflags_host', stdout=expect)

test.sleep()

try:
  PushEnv()
  os.environ['CFLAGS'] = ''
  test.run_gyp('cflags.gyp')
  test.build('cflags.gyp')
finally:
  # We clear the environ after calling gyp.  When the auto-regeneration happens,
  # the same define should be reused anyway.  Reset to empty string first in
  # case the platform doesn't support unsetenv.
  PopEnv()


expect = """FOO not defined\n"""
test.run_built_executable('cflags', stdout=expect)

test.sleep()

try:
  PushEnv()
  os.environ['CFLAGS'] = '-DFOO=1'
  test.run_gyp('cflags.gyp')
  test.build('cflags.gyp')
finally:
  # We clear the environ after calling gyp.  When the auto-regeneration happens,
  # the same define should be reused anyway.  Reset to empty string first in
  # case the platform doesn't support unsetenv.
  PopEnv()


expect = """FOO defined\n"""
test.run_built_executable('cflags', stdout=expect)

test.pass_test()
