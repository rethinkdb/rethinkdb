# Status: ported.
# Base revision: 64488.

# Copyright 2003 Dave Abrahams 
# Copyright 2002, 2003 Rene Rivera 
# Copyright 2002, 2003, 2004, 2005 Vladimir Prus 
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt) 

# Defines the "symlink" special target. 'symlink' targets make symbolic links
# to the sources.

import b2.build.feature as feature
import b2.build.targets as targets
import b2.build.property_set as property_set
import b2.build.virtual_target as virtual_target
import b2.build.targets

from b2.manager import get_manager

import bjam

import os


feature.feature("symlink-location", ["project-relative", "build-relative"], ["incidental"])

class SymlinkTarget(targets.BasicTarget):

    _count = 0

    def __init__(self, project, targets, sources):
         
        # Generate a fake name for now. Need unnamed targets eventually.
        fake_name = "symlink#%s" % SymlinkTarget._count
        SymlinkTarget._count = SymlinkTarget._count + 1

        b2.build.targets.BasicTarget.__init__(self, fake_name, project, sources)
    
        # Remember the targets to map the sources onto. Pad or truncate
        # to fit the sources given.
        assert len(targets) <= len(sources)
        self.targets = targets[:] + sources[len(targets):]
            
        # The virtual targets corresponding to the given targets.
        self.virtual_targets = []

    def construct(self, name, source_targets, ps):
        i = 0
        for t in source_targets:
            s = self.targets[i]
            a = virtual_target.Action(self.manager(), [t], "symlink.ln", ps)
            vt = virtual_target.FileTarget(os.path.basename(s), t.type(), self.project(), a)
                        
            # Place the symlink in the directory relative to the project
            # location, instead of placing it in the build directory.
            if not ps.get('symlink-location') == "project-relative":
                vt.set_path(os.path.join(self.project().get('location'), os.path.dirname(s)))

            vt = get_manager().virtual_targets().register(vt)
            self.virtual_targets.append(vt)
            i = i + 1

        return (property_set.empty(), self.virtual_targets)

# Creates a symbolic link from a set of targets to a set of sources.
# The targets and sources map one to one. The symlinks generated are
# limited to be the ones given as the sources. That is, the targets
# are either padded or trimmed to equate to the sources. The padding
# is done with the name of the corresponding source. For example::
#
#     symlink : one two ;
#
# Is equal to::
#
#     symlink one two : one two ;
#
# Names for symlink are relative to the project location. They cannot
# include ".." path components.
def symlink(targets, sources):

    from b2.manager import get_manager
    t = get_manager().targets()   
    p = get_manager().projects().current()

    return t.main_target_alternative(
        SymlinkTarget(p, targets, 
                      # Note: inline targets are not supported for symlink, intentionally,
                      # since it's used to linking existing non-local targets.
                      sources))


def setup_ln(targets, sources, ps):

    source_path = bjam.call("get-target-variable", sources[0], "LOCATE")[0]
    target_path = bjam.call("get-target-variable", targets[0], "LOCATE")[0]
    rel = os.path.relpath(source_path, target_path)
    if rel == ".":
        bjam.call("set-target-variable", targets, "PATH_TO_SOURCE", "")
    else:
        bjam.call("set-target-variable", targets, "PATH_TO_SOURCE", rel)

if os.name == 'nt':
    ln_action = """echo "NT symlinks not supported yet, making copy"
del /f /q "$(<)" 2>nul >nul
copy "$(>)" "$(<)" $(NULL_OUT)"""
else:
    ln_action = "ln -f -s '$(>:D=:R=$(PATH_TO_SOURCE))' '$(<)'"

get_manager().engine().register_action("symlink.ln", ln_action, function=setup_ln)

get_manager().projects().add_rule("symlink", symlink)
