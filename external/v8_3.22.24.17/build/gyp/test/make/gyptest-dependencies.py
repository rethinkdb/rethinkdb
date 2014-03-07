#!/usr/bin/env python

# Copyright (c) 2012 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Verifies that .d files and all.deps are properly generated.
"""

import TestGyp

# .d files are only used by the make build.
test = TestGyp.TestGyp(formats=['make'])

test.run_gyp('dependencies.gyp')

test.build('dependencies.gyp', test.ALL)

deps_file = test.built_file_path(".deps/out/Default/obj.target/main/main.o.d")
test.must_contain(deps_file, "main.h")

# Build a second time to make sure we generate all.deps.
test.build('dependencies.gyp', test.ALL)

test.pass_test()
