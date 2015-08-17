#!/usr/bin/env python
# Copyright (c) 2012 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
Verifies that make_global_settings can be used to override the
compiler settings.
"""

import TestGyp
import os
import copy
import sys
from string import Template


if sys.platform == 'win32':
  # cross compiling not support by ninja on windows
  # and make not supported on windows at all.
  sys.exit(0)

test = TestGyp.TestGyp(formats=['ninja', 'make'])

gypfile = 'compiler-global-settings.gyp'

replacements = { 'PYTHON': '/usr/bin/python', 'PWD': os.getcwd()}

# Process the .in gyp file to produce the final gyp file
# since we need to include absolute paths in the make_global_settings
# section.
replacements['TOOLSET'] = 'target'
s = Template(open(gypfile + '.in').read())
output = open(gypfile, 'w')
output.write(s.substitute(replacements))
output.close()

old_env = dict(os.environ)
os.environ['GYP_CROSSCOMPILE'] = '1'
test.run_gyp(gypfile)
os.environ.clear()
os.environ.update(old_env)

test.build(gypfile)
test.must_contain_all_lines(test.stdout(), ['my_cc.py', 'my_cxx.py', 'FOO'])

# Same again but with the host toolset.
replacements['TOOLSET'] = 'host'
s = Template(open(gypfile + '.in').read())
output = open(gypfile, 'w')
output.write(s.substitute(replacements))
output.close()

old_env = dict(os.environ)
os.environ['GYP_CROSSCOMPILE'] = '1'
test.run_gyp(gypfile)
os.environ.clear()
os.environ.update(old_env)

test.build(gypfile)
test.must_contain_all_lines(test.stdout(), ['my_cc.py', 'my_cxx.py', 'BAR'])

# Check that CC_host overrides make_global_settings
old_env = dict(os.environ)
os.environ['CC_host'] = '%s %s/my_cc.py SECRET' % (replacements['PYTHON'],
                                                   replacements['PWD'])
test.run_gyp(gypfile)
os.environ.clear()
os.environ.update(old_env)

test.build(gypfile)
test.must_contain_all_lines(test.stdout(), ['SECRET', 'my_cxx.py', 'BAR'])

test.pass_test()
