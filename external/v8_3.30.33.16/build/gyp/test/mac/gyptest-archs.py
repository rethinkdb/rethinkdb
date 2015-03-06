#!/usr/bin/env python

# Copyright (c) 2012 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Tests things related to ARCHS.
"""

import TestGyp

import re
import subprocess
import sys

if sys.platform == 'darwin':
  test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'])

  def GetStdout(cmdlist):
    proc = subprocess.Popen(cmdlist, stdout=subprocess.PIPE)
    o = proc.communicate()[0].strip()
    assert not proc.returncode
    return o

  def CheckFileType(file, expected):
    o = GetStdout(['file', '-b', file])
    if not re.match(expected, o, re.DOTALL):
      print 'File: Expected %s, got %s' % (expected, o)
      test.fail_test()

  def XcodeVersion():
    xcode, build = GetStdout(['xcodebuild', '-version']).splitlines()
    xcode = xcode.split()[-1].replace('.', '')
    xcode = (xcode + '0' * (3 - len(xcode))).zfill(4)
    return xcode

  test.run_gyp('test-no-archs.gyp', chdir='archs')
  test.build('test-no-archs.gyp', test.ALL, chdir='archs')
  result_file = test.built_file_path('Test', chdir='archs')
  test.must_exist(result_file)

  if XcodeVersion() >= '0500':
    expected_type = '^Mach-O 64-bit executable x86_64$'
  else:
    expected_type = '^Mach-O executable i386$'
  CheckFileType(result_file, expected_type)

  test.run_gyp('test-archs-x86_64.gyp', chdir='archs')
  test.build('test-archs-x86_64.gyp', test.ALL, chdir='archs')
  result_file = test.built_file_path('Test64', chdir='archs')
  test.must_exist(result_file)
  CheckFileType(result_file, '^Mach-O 64-bit executable x86_64$')

  if test.format != 'make':
    test.run_gyp('test-archs-multiarch.gyp', chdir='archs')
    test.build('test-archs-multiarch.gyp', test.ALL, chdir='archs')

    result_file = test.built_file_path(
        'static_32_64', chdir='archs', type=test.STATIC_LIB)
    test.must_exist(result_file)
    CheckFileType(result_file, 'Mach-O universal binary with 2 architectures'
                               '.*architecture i386.*architecture x86_64')

    result_file = test.built_file_path(
        'shared_32_64', chdir='archs', type=test.SHARED_LIB)
    test.must_exist(result_file)
    CheckFileType(result_file, 'Mach-O universal binary with 2 architectures'
                               '.*architecture i386.*architecture x86_64')

    result_file = test.built_file_path(
        'exe_32_64', chdir='archs', type=test.EXECUTABLE)
    test.must_exist(result_file)
    CheckFileType(result_file, 'Mach-O universal binary with 2 architectures'
                               '.*architecture i386.*architecture x86_64')

    result_file = test.built_file_path('Test App.app/Contents/MacOS/Test App',
                                       chdir='archs')
    test.must_exist(result_file)
    CheckFileType(result_file, 'Mach-O universal binary with 2 architectures'
                               '.*architecture i386.*architecture x86_64')
