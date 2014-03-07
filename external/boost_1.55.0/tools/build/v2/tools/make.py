# Status: ported.
# Base revision: 64068

# Copyright 2003 Dave Abrahams 
# Copyright 2003 Douglas Gregor 
# Copyright 2006 Rene Rivera 
# Copyright 2002, 2003, 2004, 2005, 2006 Vladimir Prus 
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt) 

#  This module defines the 'make' main target rule.

from b2.build.targets import BasicTarget
from b2.build.virtual_target import Action, FileTarget
from b2.build import type
from b2.manager import get_manager
import b2.build.property_set


class MakeTarget(BasicTarget):
  
    def construct(self, name, source_targets, property_set):

        action_name = property_set.get("<action>")[0]            
        action = Action(get_manager(), source_targets, action_name[1:], property_set)
        target = FileTarget(self.name(), type.type(self.name()),
                            self.project(), action, exact=True)    
        return [ b2.build.property_set.empty(),
                 [self.project().manager().virtual_targets().register(target)]]

def make (target_name, sources, generating_rule,
          requirements=None, usage_requirements=None):

    target_name = target_name[0]
    generating_rule = generating_rule[0]
    if generating_rule[0] != '@':
        generating_rule = '@' + generating_rule

    if not requirements:
        requirements = []

        
    requirements.append("<action>%s" % generating_rule)
    
    m = get_manager()
    targets = m.targets()
    project = m.projects().current()
    engine = m.engine()
    engine.register_bjam_action(generating_rule)

    targets.main_target_alternative(MakeTarget(
        target_name, project,
        targets.main_target_sources(sources, target_name),
        targets.main_target_requirements(requirements, project),
        targets.main_target_default_build([], project),
        targets.main_target_usage_requirements(usage_requirements or [], project)))

get_manager().projects().add_rule("make", make)

