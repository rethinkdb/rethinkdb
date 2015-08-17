#!/usr/bin/env python

# Copyright (c) 2011 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Verifies rules related variables are expanded.
"""

import TestGyp

test = TestGyp.TestGyp(formats=['ninja'])

test.relocate('src', 'relocate/src')

test.run_gyp('variables.gyp', chdir='relocate/src')

test.build('variables.gyp', chdir='relocate/src')

test.run_built_executable('all_rule_variables',
                          chdir='relocate/src',
                          stdout="input_root\ninput_dirname\ninput_path\n" +
                          "input_ext\ninput_name\n")

test.pass_test()
