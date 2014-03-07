#!/usr/bin/env python

# Copyright Aleksey Gurtovoy 2004-2009
#
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)


import locale
try:
    locale.setlocale(locale.LC_ALL, '')
except:
    pass

from docutils.parsers.rst import directives
from docutils.parsers.rst.directives import htmlrefdoc
directives.register_directive( 'copyright', htmlrefdoc.LicenseAndCopyright )

from docutils.core import publish_cmdline, default_description

description = ('Generates "framed" (X)HTML documents from standalone reStructuredText '
               'sources.  ' + default_description)

publish_cmdline(writer_name='html4_refdoc', description=description)
