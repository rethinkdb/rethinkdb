# Status: ported.
# Base revision: 64429.
#
#  Copyright (c) 2005-2010 Vladimir Prus.
#
#  Use, modification and distribution is subject to the Boost Software
#  License Version 1.0. (See accompanying file LICENSE_1_0.txt or
#  http://www.boost.org/LICENSE_1_0.txt)


import b2.build.type as type
import b2.build.generators as generators
import b2.build.virtual_target as virtual_target
import b2.build.toolset as toolset
import b2.build.targets as targets

from b2.manager import get_manager
from b2.util import bjam_signature

type.register("NOTFILE_MAIN")

class NotfileGenerator(generators.Generator):

    def run(self, project, name, ps, sources):
        pass
        action_name = ps.get('action')[0]
        if action_name[0] == '@':
            action = virtual_target.Action(get_manager(), sources, action_name[1:], ps)
        else:
            action = virtual_target.Action(get_manager(), sources, "notfile.run", ps)

        return [get_manager().virtual_targets().register(
            virtual_target.NotFileTarget(name, project, action))]

generators.register(NotfileGenerator("notfile.main", False, [], ["NOTFILE_MAIN"]))

toolset.flags("notfile.run", "ACTION", [], ["<action>"])

get_manager().engine().register_action("notfile.run", "$(ACTION)")

@bjam_signature((["target_name"], ["action"], ["sources", "*"], ["requirements", "*"],
                 ["default_build", "*"]))
def notfile(target_name, action, sources, requirements, default_build):

    requirements.append("<action>" + action)

    return targets.create_typed_metatarget(target_name, "NOTFILE_MAIN", sources, requirements,
                                           default_build, [])


get_manager().projects().add_rule("notfile", notfile)
