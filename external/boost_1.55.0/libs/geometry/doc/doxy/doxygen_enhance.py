#! /usr/bin/env python
# -*- coding: utf-8 -*-
# ===========================================================================
#  Copyright (c) 2010 Barend Gehrels, Amsterdam, the Netherlands.
# 
#  Use, modification and distribution is subject to the Boost Software License,
#  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)9
# ============================================================================

import sys

args = sys.argv[1:]
if len(args) != 1:
    raise SystemExit("Usage: doxygen_enhance <html filename>")


# 1) set variable for doxygen_contents to be posted
file_in = open(args[0], 'r')
doxygen_contents = file_in.read()

doxygen_contents = doxygen_contents.replace('<span>Modules</span>', '<span>Function overview</span>')
doxygen_contents = doxygen_contents.replace('<span>Related&nbsp;Pages</span>', '<span>Main pages</span>')
doxygen_contents = doxygen_contents.replace('<span>Main&nbsp;Page</span>', '<span>Introduction</span>')
doxygen_contents = doxygen_contents.replace('<li><a href="namespaces.html"><span>Namespaces</span></a></li>', '')

doxygen_contents = doxygen_contents.replace('<h1>Modules</h1>Here is a list of all modules:<ul>', '<h1>Function overview</h1>Here is a list of all functions:<ul>')
doxygen_contents = doxygen_contents.replace('<h1>Related Pages</h1>Here is a list of all related documentation pages:<ul>', '<h1>Main pages</h1>The following pages give backgrounds on Boost.Geometry:<ul>')


file_out = file (args[0], 'w')
file_out.write(doxygen_contents)
file_out.close()
