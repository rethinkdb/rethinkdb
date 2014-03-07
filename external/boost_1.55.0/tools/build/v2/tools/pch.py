# Status: Being ported by Steven Watanabe
# Base revision: 47077
#
# Copyright (c) 2005 Reece H. Dunn.
# Copyright 2006 Ilya Sokolov
# Copyright (c) 2008 Steven Watanabe
#
# Use, modification and distribution is subject to the Boost Software
# License Version 1.0. (See accompanying file LICENSE_1_0.txt or
# http://www.boost.org/LICENSE_1_0.txt)

##### Using Precompiled Headers (Quick Guide) #####
#
# Make precompiled mypch.hpp:
#
#    import pch ;
#
#    cpp-pch mypch
#      : # sources
#        mypch.hpp
#      : # requiremnts
#        <toolset>msvc:<source>mypch.cpp
#      ;
#
# Add cpp-pch to sources:
#
#    exe hello
#      : main.cpp hello.cpp mypch
#      ;

from b2.build import type, feature, generators
from b2.tools import builtin

type.register('PCH', ['pch'])
type.register('C_PCH', [], 'PCH')
type.register('CPP_PCH', [], 'PCH')

# Control precompiled header (PCH) generation.
feature.feature('pch',
                ['on', 'off'],
                ['propagated'])

feature.feature('pch-header', [], ['free', 'dependency'])
feature.feature('pch-file', [], ['free', 'dependency'])

class PchGenerator(generators.Generator):
    """
        Base PCH generator. The 'run' method has the logic to prevent this generator
        from being run unless it's being used for a top-level PCH target.
    """
    def action_class(self):
        return builtin.CompileAction

    def run(self, project, name, prop_set, sources):
        if not name:
            # Unless this generator is invoked as the top-most generator for a
            # main target, fail. This allows using 'H' type as input type for
            # this generator, while preventing Boost.Build to try this generator
            # when not explicitly asked for.
            #
            # One bad example is msvc, where pch generator produces both PCH
            # target and OBJ target, so if there's any header generated (like by
            # bison, or by msidl), we'd try to use pch generator to get OBJ from
            # that H, which is completely wrong. By restricting this generator
            # only to pch main target, such problem is solved.
            pass
        else:
            r = self.run_pch(project, name,
                 prop_set.add_raw(['<define>BOOST_BUILD_PCH_ENABLED']),
                 sources)
            return generators.add_usage_requirements(
                r, ['<define>BOOST_BUILD_PCH_ENABLED'])

    # This rule must be overridden by the derived classes.
    def run_pch(self, project, name, prop_set, sources):
        pass

# NOTE: requirements are empty, default pch generator can be applied when
# pch=off.
generators.register(builtin.DummyGenerator(
    "pch.default-c-pch-generator", False, [], ['C_PCH'], []))
generators.register(builtin.DummyGenerator(
    "pch.default-cpp-pch-generator", False, [], ['CPP_PCH'], []))
