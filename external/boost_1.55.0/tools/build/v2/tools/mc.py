# Copyright (c) 2005 Alexey Pakhunov.
# Copyright (c) 2011 Juraj Ivancic
#
# Use, modification and distribution is subject to the Boost Software
# License Version 1.0. (See accompanying file LICENSE_1_0.txt or
# http://www.boost.org/LICENSE_1_0.txt)

#  Support for Microsoft message compiler tool.
#  Notes:
#  - there's just message compiler tool, there's no tool for 
#    extracting message strings from sources
#  - This file allows to use Microsoft message compiler
#    with any toolset. In msvc.jam, there's more specific
#    message compiling action.

import bjam

from b2.tools import common, rc
from b2.build import generators, type
from b2.build.toolset import flags
from b2.build.feature import feature
from b2.manager import get_manager

def init():
    pass

type.register('MC', ['mc'])


# Command line options
feature('mc-input-encoding', ['ansi', 'unicode'], ['free'])
feature('mc-output-encoding', ['unicode', 'ansi'], ['free'])
feature('mc-set-customer-bit', ['no', 'yes'], ['free'])

flags('mc.compile', 'MCFLAGS', ['<mc-input-encoding>ansi'], ['-a'])
flags('mc.compile', 'MCFLAGS', ['<mc-input-encoding>unicode'], ['-u'])
flags('mc.compile', 'MCFLAGS', ['<mc-output-encoding>ansi'], '-A')
flags('mc.compile', 'MCFLAGS', ['<mc-output-encoding>unicode'], ['-U'])
flags('mc.compile', 'MCFLAGS', ['<mc-set-customer-bit>no'], [])
flags('mc.compile', 'MCFLAGS', ['<mc-set-customer-bit>yes'], ['-c'])

generators.register_standard('mc.compile', ['MC'], ['H', 'RC'])

get_manager().engine().register_action(
    'mc.compile',
    'mc $(MCFLAGS) -h "$(<[1]:DW)" -r "$(<[2]:DW)" "$(>:W)"')
