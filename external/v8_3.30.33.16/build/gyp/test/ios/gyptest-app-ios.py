#!/usr/bin/env python

# Copyright (c) 2012 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Verifies that ios app bundles are built correctly.
"""

import TestGyp

import sys

if sys.platform == 'darwin':
  test = TestGyp.TestGyp(formats=['xcode', 'ninja'])

  test.run_gyp('test.gyp', chdir='app-bundle')

  test.build('test.gyp', test.ALL, chdir='app-bundle')

  # Test that the extension is .bundle
  test.built_file_must_exist('Test App Gyp.bundle/Test App Gyp',
                             chdir='app-bundle')

  # Info.plist
  info_plist = test.built_file_path('Test App Gyp.bundle/Info.plist',
                                    chdir='app-bundle')
  # Resources
  test.built_file_must_exist(
      'Test App Gyp.bundle/English.lproj/InfoPlist.strings',
      chdir='app-bundle')
  test.built_file_must_exist(
      'Test App Gyp.bundle/English.lproj/MainMenu.nib',
      chdir='app-bundle')
  test.built_file_must_exist(
      'Test App Gyp.bundle/English.lproj/Main_iPhone.storyboardc',
      chdir='app-bundle')

  # Packaging
  test.built_file_must_exist('Test App Gyp.bundle/PkgInfo',
                             chdir='app-bundle')
  test.built_file_must_match('Test App Gyp.bundle/PkgInfo', 'APPLause',
                             chdir='app-bundle')

  test.pass_test()
