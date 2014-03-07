# Status: ported.
# Base revision 64444.
#
# Copyright 2003 Dave Abrahams
# Copyright 2005, 2006 Rene Rivera
# Copyright 2002, 2003, 2004, 2005, 2006, 2010 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# This module defines the 'install' rule, used to copy a set of targets to a
# single location.

import b2.build.feature as feature
import b2.build.targets as targets
import b2.build.property as property
import b2.build.property_set as property_set
import b2.build.generators as generators
import b2.build.virtual_target as virtual_target

from b2.manager import get_manager
from b2.util.sequence import unique
from b2.util import bjam_signature

import b2.build.type

import os.path
import re
import types

feature.feature('install-dependencies', ['off', 'on'], ['incidental'])
feature.feature('install-type', [], ['free', 'incidental'])
feature.feature('install-source-root', [], ['free', 'path'])
feature.feature('so-version', [], ['free', 'incidental'])

# If 'on', version symlinks for shared libraries will not be created. Affects
# Unix builds only.
feature.feature('install-no-version-symlinks', ['on'], ['optional', 'incidental'])

class InstallTargetClass(targets.BasicTarget):

    def update_location(self, ps):
        """If <location> is not set, sets it based on the project data."""

        loc = ps.get('location')
        if not loc:
            loc = os.path.join(self.project().get('location'), self.name())
            ps = ps.add_raw(["<location>" + loc])

        return ps

    def adjust_properties(self, target, build_ps):
        a = target.action()
        properties = []
        if a:
            ps = a.properties()
            properties = ps.all()
            
            # Unless <hardcode-dll-paths>true is in properties, which can happen
            # only if the user has explicitly requested it, nuke all <dll-path>
            # properties.

            if build_ps.get('hardcode-dll-paths') != ['true']:
                properties = [p for p in properties if p.feature().name() != 'dll-path']

            # If any <dll-path> properties were specified for installing, add
            # them.
            properties.extend(build_ps.get_properties('dll-path'))

            # Also copy <linkflags> feature from current build set, to be used
            # for relinking.
            properties.extend(build_ps.get_properties('linkflags'))

            # Remove the <tag> feature on original targets.
            # And <location>. If stage target has another stage target in
            # sources, then we shall get virtual targets with the <location>
            # property set.
            properties = [p for p in properties
                          if not p.feature().name() in ['tag', 'location']]

        properties.extend(build_ps.get_properties('dependency'))

        properties.extend(build_ps.get_properties('location'))
        

        properties.extend(build_ps.get_properties('install-no-version-symlinks'))

        d = build_ps.get_properties('install-source-root')

        # Make the path absolute: we shall use it to compute relative paths and
        # making the path absolute will help.
        if d:
            p = d[0]
            properties.append(property.Property(p.feature(), os.path.abspath(p.value())))

        return property_set.create(properties)
    

    def construct(self, name, source_targets, ps):

        source_targets = self.targets_to_stage(source_targets, ps)
        ps = self.update_location(ps)

        ename = ps.get('name')
        if ename:
            ename = ename[0]
        if ename and len(source_targets) > 1:
            get_manager().errors()("When <name> property is used in 'install', only one source is allowed")

        result = []

        for i in source_targets:

            staged_targets = []
            new_ps = self.adjust_properties(i, ps)

            # See if something special should be done when staging this type. It
            # is indicated by the presence of a special "INSTALLED_" type.
            t = i.type()
            if t and b2.build.type.registered("INSTALLED_" + t):

                if ename:
                    get_manager().errors()("In 'install': <name> property specified with target that requires relinking.")
                else:
                    (r, targets) = generators.construct(self.project(), name, "INSTALLED_" + t,
                                                        new_ps, [i])
                    assert isinstance(r, property_set.PropertySet)
                    staged_targets.extend(targets)
                    
            else:
                staged_targets.append(copy_file(self.project(), ename, i, new_ps))

            if not staged_targets:
                get_manager().errors()("Unable to generate staged version of " + i)

            result.extend(get_manager().virtual_targets().register(t) for t in staged_targets)

        return (property_set.empty(), result)

    def targets_to_stage(self, source_targets, ps):
        """Given the list of source targets explicitly passed to 'stage', returns the
        list of targets which must be staged."""

        result = []

        # Traverse the dependencies, if needed.
        if ps.get('install-dependencies') == ['on']:
            source_targets = self.collect_targets(source_targets)

        # Filter the target types, if needed.
        included_types = ps.get('install-type')
        for r in source_targets:
            ty = r.type()
            if ty:
                # Do not stage searched libs.
                if ty != "SEARCHED_LIB":
                    if included_types:
                        if self.include_type(ty, included_types):
                            result.append(r)
                    else:
                        result.append(r)
            elif not included_types:
                # Don't install typeless target if there is an explicit list of
                # allowed types.
                result.append(r)

        return result

    # CONSIDER: figure out why we can not use virtual-target.traverse here.
    #
    def collect_targets(self, targets):
        
        s = [t.creating_subvariant() for t in targets]
        s = unique(s)
        
        result = set(targets)
        for i in s:
            i.all_referenced_targets(result)
           
        result2 = []
        for r in result:
            if isinstance(r, property.Property):
                
                if r.feature().name() != 'use':
                    result2.append(r.value())
            else:
                result2.append(r)
        result2 = unique(result2)
        return result2

    # Returns true iff 'type' is subtype of some element of 'types-to-include'.
    #
    def include_type(self, type, types_to_include):
        return any(b2.build.type.is_subtype(type, ti) for ti in types_to_include)

# Creates a copy of target 'source'. The 'properties' object should have a
# <location> property which specifies where the target must be placed.
#
def copy_file(project, name, source, ps):

    if not name:
        name = source.name()

    relative = ""

    new_a = virtual_target.NonScanningAction([source], "common.copy", ps)
    source_root = ps.get('install-source-root')
    if source_root:
        source_root = source_root[0]
        # Get the real path of the target. We probably need to strip relative
        # path from the target name at construction.
        path = os.path.join(source.path(), os.path.dirname(name))
        # Make the path absolute. Otherwise, it would be hard to compute the
        # relative path. The 'source-root' is already absolute, see the
        # 'adjust-properties' method above.
        path = os.path.abspath(path)

        relative = os.path.relpath(path, source_root)

    name = os.path.join(relative, os.path.basename(name))
    return virtual_target.FileTarget(name, source.type(), project, new_a, exact=True)

def symlink(name, project, source, ps):
    a = virtual_target.Action([source], "symlink.ln", ps)
    return virtual_target.FileTarget(name, source.type(), project, a, exact=True)

def relink_file(project, source, ps):
    action = source[0].action()
    cloned_action = virtual_target.clone_action(action, project, "", ps)
    targets = cloned_action.targets()
    # We relink only on Unix, where exe or shared lib is always a single file.
    assert len(targets) == 1
    return targets[0]


# Declare installed version of the EXE type. Generator for this type will cause
# relinking to the new location.
b2.build.type.register('INSTALLED_EXE', [], 'EXE')

class InstalledExeGenerator(generators.Generator):

    def __init__(self):
        generators.Generator.__init__(self, "install-exe", False, ['EXE'], ['INSTALLED_EXE'])

    def run(self, project, name, ps, source):

        need_relink = False;

        if ps.get('os') in ['NT', 'CYGWIN'] or ps.get('target-os') in ['windows', 'cygwin']:
            # Never relink
            pass
        else:
            # See if the dll-path properties are not changed during
            # install. If so, copy, don't relink.
            need_relink = ps.get('dll-path') != source[0].action().properties().get('dll-path')

        if need_relink:
            return [relink_file(project, source, ps)]
        else:
            return [copy_file(project, None, source[0], ps)]

generators.register(InstalledExeGenerator())


# Installing a shared link on Unix might cause a creation of versioned symbolic
# links.
b2.build.type.register('INSTALLED_SHARED_LIB', [], 'SHARED_LIB')

class InstalledSharedLibGenerator(generators.Generator):

    def __init__(self):
        generators.Generator.__init__(self, 'install-shared-lib', False, ['SHARED_LIB'], ['INSTALLED_SHARED_LIB'])

    def run(self, project, name, ps, source):

        source = source[0]
        if ps.get('os') in ['NT', 'CYGWIN'] or ps.get('target-os') in ['windows', 'cygwin']:
            copied = copy_file(project, None, source, ps)
            return [get_manager().virtual_targets().register(copied)]
        else:
            a = source.action()
            if not a:
                # Non-derived file, just copy.
                copied = copy_file(project, source, ps)
            else:

                need_relink = ps.get('dll-path') != source.action().properties().get('dll-path')
                
                if need_relink:
                    # Rpath changed, need to relink.
                    copied = relink_file(project, source, ps)
                else:
                    copied = copy_file(project, None, source, ps)

            result = [get_manager().virtual_targets().register(copied)]
            # If the name is in the form NNN.XXX.YYY.ZZZ, where all 'X', 'Y' and
            # 'Z' are numbers, we need to create NNN.XXX and NNN.XXX.YYY
            # symbolic links.
            m = re.match("(.*)\\.([0123456789]+)\\.([0123456789]+)\\.([0123456789]+)$",
                         copied.name());
            if m:
                # Symlink without version at all is used to make
                # -lsome_library work.
                result.append(symlink(m.group(1), project, copied, ps))

                # Symlinks of some libfoo.N and libfoo.N.M are used so that
                # library can found at runtime, if libfoo.N.M.X has soname of
                # libfoo.N. That happens when the library makes some binary
                # compatibility guarantees. If not, it is possible to skip those
                # symlinks.
                if ps.get('install-no-version-symlinks') != ['on']:
                
                    result.append(symlink(m.group(1) + '.' + m.group(2), project, copied, ps))
                    result.append(symlink(m.group(1) + '.' + m.group(2) + '.' + m.group(3),
                                          project, copied, ps))

            return result
            
generators.register(InstalledSharedLibGenerator())


# Main target rule for 'install'.
#
@bjam_signature((["name"], ["sources", "*"], ["requirements", "*"],
                 ["default_build", "*"], ["usage_requirements", "*"]))
def install(name, sources, requirements=[], default_build=[], usage_requirements=[]):

    requirements = requirements[:]
    # Unless the user has explicitly asked us to hardcode dll paths, add
    # <hardcode-dll-paths>false in requirements, to override default value.
    if not '<hardcode-dll-paths>true' in requirements:
        requirements.append('<hardcode-dll-paths>false')

    if any(r.startswith('<tag>') for r in requirements):
        get_manager().errors()("The <tag> property is not allowed for the 'install' rule")

    from b2.manager import get_manager
    t = get_manager().targets()
    
    project = get_manager().projects().current()
        
    return t.main_target_alternative(
        InstallTargetClass(name, project,
                           t.main_target_sources(sources, name),
                           t.main_target_requirements(requirements, project),
                           t.main_target_default_build(default_build, project),
                           t.main_target_usage_requirements(usage_requirements, project)))

get_manager().projects().add_rule("install", install)
get_manager().projects().add_rule("stage", install)

