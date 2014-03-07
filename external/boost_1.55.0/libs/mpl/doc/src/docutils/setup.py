#!/usr/bin/env python

# Copyright Aleksey Gurtovoy 2007-2009
#
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)


import sys, os
from distutils.core import setup

setup(
    name="html_refdoc",
    version=".1",
    description="convert C++ rst documentation to a set of HTML pages/frames.",
    author="Aleksey Gurtovoy",
    author_email="agurtovoy@meta-comm.com",
    packages=['docutils.writers.html4_refdoc', 'docutils.parsers.rst.directives'],
    package_dir={'docutils.writers.html4_refdoc': 'writers/html4_refdoc'
                ,'docutils.parsers.rst.directives': 'parsers/rst/directives' },
    package_data={'docutils.writers.html4_refdoc': ['frames.css']},
    scripts=["tools/rst2htmlrefdoc.py"],
    )
