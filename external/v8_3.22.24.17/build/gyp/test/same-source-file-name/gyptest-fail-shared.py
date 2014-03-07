#!/usr/bin/env python

# Copyright (c) 2012 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Checks that gyp fails on shared_library targets which have several files with
the same basename.
"""

import TestGyp

test = TestGyp.TestGyp()

test.run_gyp('double-shared.gyp', chdir='src', status=1, stderr=None)

test.pass_test()
