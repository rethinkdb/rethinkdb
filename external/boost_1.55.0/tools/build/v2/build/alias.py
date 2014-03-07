# Copyright 2003, 2004, 2006 Vladimir Prus 
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt) 

# Status: ported (danielw)
# Base revision: 56043

#  This module defines the 'alias' rule and associated class.
#
#  Alias is just a main target which returns its source targets without any 
#  processing. For example::
#
#    alias bin : hello test_hello ;
#    alias lib : helpers xml_parser ;
#
#  Another important use of 'alias' is to conveniently group source files::
#
#    alias platform-src : win.cpp : <os>NT ;
#    alias platform-src : linux.cpp : <os>LINUX ;
#    exe main : main.cpp platform-src ;
# 
#  Lastly, it's possible to create local alias for some target, with different
#  properties::
#
#    alias big_lib : : @/external_project/big_lib/<link>static ;
#

import targets
import property_set
from b2.manager import get_manager

from b2.util import metatarget

class AliasTarget(targets.BasicTarget):

    def __init__(self, *args):
        targets.BasicTarget.__init__(self, *args)

    def construct(self, name, source_targets, properties):
        return [property_set.empty(), source_targets]

    def compute_usage_requirements(self, subvariant):
        base = targets.BasicTarget.compute_usage_requirements(self, subvariant)
        # Add source's usage requirement. If we don't do this, "alias" does not
        # look like 100% alias.
        return base.add(subvariant.sources_usage_requirements())

@metatarget
def alias(name, sources=[], requirements=[], default_build=[], usage_requirements=[]):

    project = get_manager().projects().current()
    targets = get_manager().targets()

    targets.main_target_alternative(AliasTarget(
        name, project,
        targets.main_target_sources(sources, name, no_renaming=True),
        targets.main_target_requirements(requirements or [], project),
        targets.main_target_default_build(default_build, project),
        targets.main_target_usage_requirements(usage_requirements or [], project)))

# Declares the 'alias' target. It will build sources, and return them unaltered.
get_manager().projects().add_rule("alias", alias)

