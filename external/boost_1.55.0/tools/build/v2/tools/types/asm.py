# Copyright Craig Rodrigues 2005.
# Copyright (c) 2008 Steven Watanabe
#
# Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

from b2.build import type

def register():
    type.register_type('ASM', ['s', 'S', 'asm'])

register()
