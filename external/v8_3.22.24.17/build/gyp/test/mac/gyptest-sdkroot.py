#!/usr/bin/env python

# Copyright (c) 2012 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Verifies that setting SDKROOT works.
"""

import TestGyp

import os
import subprocess
import sys

if sys.platform == 'darwin':
  test = TestGyp.TestGyp(formats=['ninja', 'make', 'xcode'])

  # Make sure this works on the bots, which only have the 10.6 sdk, and on
  # dev machines, who usually don't have the 10.6 sdk.
  sdk = '10.6'
  DEVNULL = open(os.devnull, 'wb')
  proc = subprocess.Popen(['xcodebuild', '-version', '-sdk', 'macosx' + sdk],
                          stdout=DEVNULL, stderr=DEVNULL)
  proc.communicate()
  DEVNULL.close()
  if proc.returncode:
    sdk = '10.7'

  proc = subprocess.Popen(['xcodebuild', '-version',
                           '-sdk', 'macosx' + sdk, 'Path'],
                          stdout=subprocess.PIPE)
  sdk_path = proc.communicate()[0].rstrip('\n')
  if proc.returncode != 0:
    test.fail_test()

  test.write('sdkroot/test.gyp', test.read('sdkroot/test.gyp') % sdk)

  test.run_gyp('test.gyp', '-D', 'sdk_path=%s' % sdk_path,
               chdir='sdkroot')
  test.build('test.gyp', test.ALL, chdir='sdkroot')
  test.pass_test()
