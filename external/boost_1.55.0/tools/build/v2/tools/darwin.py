#  Copyright (C) Christopher Currie 2003. Permission to copy, use,
#  modify, sell and distribute this software is granted provided this
#  copyright notice appears in all copies. This software is provided
# "as is" without express or implied warranty, and with no claim as to
#  its suitability for any purpose.

#  Please see http://article.gmane.org/gmane.comp.lib.boost.build/3389/
#  for explanation why it's a separate toolset.

import common, gcc, builtin
from b2.build import feature, toolset, type, action, generators
from b2.util.utility import *

toolset.register ('darwin')

toolset.inherit_generators ('darwin', [], 'gcc')
toolset.inherit_flags ('darwin', 'gcc')
toolset.inherit_rules ('darwin', 'gcc')

def init (version = None, command = None, options = None):
    options = to_seq (options)

    condition = common.check_init_parameters ('darwin', None, ('version', version))
    
    command = common.get_invocation_command ('darwin', 'g++', command)

    common.handle_options ('darwin', condition, command, options)
    
    gcc.init_link_flags ('darwin', 'darwin', condition)

# Darwin has a different shared library suffix
type.set_generated_target_suffix ('SHARED_LIB', ['<toolset>darwin'], 'dylib')

# we need to be able to tell the type of .dylib files
type.register_suffixes ('dylib', 'SHARED_LIB')

feature.feature ('framework', [], ['free'])

toolset.flags ('darwin.compile', 'OPTIONS', '<link>shared', ['-dynamic'])
toolset.flags ('darwin.compile', 'OPTIONS', None, ['-Wno-long-double', '-no-cpp-precomp'])
toolset.flags ('darwin.compile.c++', 'OPTIONS', None, ['-fcoalesce-templates'])

toolset.flags ('darwin.link', 'FRAMEWORK', '<framework>')

# This is flag is useful for debugging the link step
# uncomment to see what libtool is doing under the hood
# toolset.flags ('darwin.link.dll', 'OPTIONS', None, '[-Wl,-v'])

action.register ('darwin.compile.cpp', None, ['$(CONFIG_COMMAND) $(ST_OPTIONS) -L"$(LINKPATH)" -o "$(<)" "$(>)" "$(LIBRARIES)" -l$(FINDLIBS-SA) -l$(FINDLIBS-ST) -framework$(_)$(FRAMEWORK) $(OPTIONS)'])

# TODO: how to set 'bind LIBRARIES'?
action.register ('darwin.link.dll', None, ['$(CONFIG_COMMAND) -dynamiclib -L"$(LINKPATH)" -o "$(<)" "$(>)" "$(LIBRARIES)" -l$(FINDLIBS-SA) -l$(FINDLIBS-ST) -framework$(_)$(FRAMEWORK) $(OPTIONS)'])

def darwin_archive (manager, targets, sources, properties):
    pass

action.register ('darwin.archive', darwin_archive, ['ar -c -r -s $(ARFLAGS) "$(<:T)" "$(>:T)"'])
