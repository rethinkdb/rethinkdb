#!/usr/bin/env python

# Copyright (c) 2013 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Verifies that device and simulator bundles are built correctly.
"""

import plistlib
import TestGyp
import os
import struct
import subprocess
import sys
import tempfile


def CheckFileType(file, expected):
  proc = subprocess.Popen(['lipo', '-info', file], stdout=subprocess.PIPE)
  o = proc.communicate()[0].strip()
  assert not proc.returncode
  if not expected in o:
    print 'File: Expected %s, got %s' % (expected, o)
    test.fail_test()


def XcodeVersion():
  xcode, build = GetStdout(['xcodebuild', '-version']).splitlines()
  xcode = xcode.split()[-1].replace('.', '')
  xcode = (xcode + '0' * (3 - len(xcode))).zfill(4)
  return xcode


def GetStdout(cmdlist):
  proc = subprocess.Popen(cmdlist, stdout=subprocess.PIPE)
  o = proc.communicate()[0].strip()
  assert not proc.returncode
  return o


if sys.platform == 'darwin':
  test = TestGyp.TestGyp()

  test.run_gyp('test-archs.gyp', chdir='app-bundle')
  test.set_configuration('Default')

  # TODO(sdefresne): add 'Test Archs x86_64' once bots have been updated to
  # a SDK version that supports "x86_64" architecture.
  filenames = ['Test No Archs', 'Test Archs i386']
  if XcodeVersion() >= '0500':
    filenames.append('Test Archs x86_64')

  for filename in filenames:
    target = filename.replace(' ', '_').lower()
    test.build('test-archs.gyp', target, chdir='app-bundle')
    result_file = test.built_file_path(
        '%s.bundle/%s' % (filename, filename), chdir='app-bundle')
    test.must_exist(result_file)

    expected = 'i386'
    if 'x86_64' in filename:
      expected = 'x86_64'
      CheckFileType(result_file, expected)

  test.pass_test()
