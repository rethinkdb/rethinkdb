# Status: ported
# Base revision: 64432.
# Copyright 2005-2010 Vladimir Prus.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Defines main target 'cast', used to change type for target. For example, in Qt
# library one wants two kinds of CPP files -- those that just compiled and those
# that are passed via the MOC tool.
#
# This is done with:
#
#    exe main : main.cpp [ cast _ moccable-cpp : widget.cpp ] ;
#
# Boost.Build will assing target type CPP to both main.cpp and widget.cpp. Then,
# the cast rule will change target type of widget.cpp to MOCCABLE-CPP, and Qt
# support will run the MOC tool as part of the build process.
#
# At the moment, the 'cast' rule only works for non-derived (source) targets.
#
# TODO: The following comment is unclear or incorrect. Clean it up.
# > Another solution would be to add a separate main target 'moc-them' that
# > would moc all the passed sources, no matter what their type is, but I prefer
# > cast, as defining a new target type + generator for that type is somewhat
# > simpler than defining a main target rule.

import b2.build.targets as targets
import b2.build.virtual_target as virtual_target

from b2.manager import get_manager
from b2.util import bjam_signature

class CastTargetClass(targets.TypedTarget):

    def construct(name, source_targets, ps):
        result = []
        for s in source_targets:
            if not isinstance(s, virtual_targets.FileTarget):
                get_manager().errors()("Source to the 'cast' metatager is not a file")

            if s.action():
                get_manager().errors()("Only non-derived targets allowed as sources for 'cast'.")


            r = s.clone_with_different_type(self.type())
            result.append(get_manager().virtual_targets().register(r))

        return result
    

@bjam_signature((["name", "type"], ["sources", "*"], ["requirements", "*"],
                 ["default_build", "*"], ["usage_requirements", "*"]))
def cast(name, type, sources, requirements, default_build, usage_requirements):
   
    from b2.manager import get_manager
    t = get_manager().targets()
    
    project = get_manager().projects().current()
        
    return t.main_target_alternative(
        CastTargetClass(name, project, type,
                        t.main_target_sources(sources, name),
                        t.main_target_requirements(requirements, project),
                        t.main_target_default_build(default_build, project),
                        t.main_target_usage_requirements(usage_requirements, project)))


get_manager().projects().add_rule("cast", cast)
