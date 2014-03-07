# Status: ported.
# Base revision: 64488.
#
# Copyright 2008, 2010 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Defines main target type 'message', that prints a message when built for the
# first time.

import b2.build.targets as targets
import b2.build.property_set as property_set

from b2.manager import get_manager

class MessageTargetClass(targets.BasicTarget):

    def __init__(self, name, project, *args):

        targets.BasicTarget.__init__(self, name, project, [])
        self.args = args
        self.built = False

    def construct(self, name, sources, ps):

        if not self.built:
            for arg in self.args:
                if type(arg) == type([]):
                    arg = " ".join(arg)                
                print arg
            self.built = True

        return (property_set.empty(), [])

def message(name, *args):

    if type(name) == type([]):
        name = name[0]

    t = get_manager().targets()
    
    project = get_manager().projects().current()
        
    return t.main_target_alternative(MessageTargetClass(*((name, project) + args)))

get_manager().projects().add_rule("message", message)
