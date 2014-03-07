# Status: mostly ported. Missing is --out-xml support, 'configure' integration
# and some FIXME.
# Base revision: 64351

# Copyright 2003, 2005 Dave Abrahams
# Copyright 2006 Rene Rivera
# Copyright 2003, 2004, 2005, 2006, 2007 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)


from b2.build.engine import Engine
from b2.manager import Manager
from b2.util.path import glob
from b2.build import feature, property_set
import b2.build.virtual_target
from b2.build.targets import ProjectTarget
from b2.util.sequence import unique
import b2.build.build_request
from b2.build.errors import ExceptionWithUserContext
import b2.tools.common
from b2.build.toolset import using

import b2.build.project as project
import b2.build.virtual_target as virtual_target
import b2.build.build_request as build_request

import b2.util.regex

from b2.manager import get_manager
from b2.util import cached
from b2.util import option


import bjam

import os
import sys
import re

################################################################################
#
# Module global data.
#
################################################################################

# Flag indicating we should display additional debugging information related to
# locating and loading Boost Build configuration files.
debug_config = False

# The cleaning is tricky. Say, if user says 'bjam --clean foo' where 'foo' is a
# directory, then we want to clean targets which are in 'foo' as well as those
# in any children Jamfiles under foo but not in any unrelated Jamfiles. To
# achieve this we collect a list of projects under which cleaning is allowed.
project_targets = []

# Virtual targets obtained when building main targets references on the command
# line. When running 'bjam --clean main_target' we want to clean only files
# belonging to that main target so we need to record which targets are produced
# for it.
results_of_main_targets = []

# Was an XML dump requested?
out_xml = False

# Default toolset & version to be used in case no other toolset has been used
# explicitly by either the loaded configuration files, the loaded project build
# scripts or an explicit toolset request on the command line. If not specified,
# an arbitrary default will be used based on the current host OS. This value,
# while not strictly necessary, has been added to allow testing Boost-Build's
# default toolset usage functionality.
default_toolset = None
default_toolset_version = None

################################################################################
#
# Public rules.
#
################################################################################

# Returns the property set with the free features from the currently processed
# build request.
#
def command_line_free_features():
    return command_line_free_features

# Sets the default toolset & version to be used in case no other toolset has
# been used explicitly by either the loaded configuration files, the loaded
# project build scripts or an explicit toolset request on the command line. For
# more detailed information see the comment related to used global variables.
#
def set_default_toolset(toolset, version=None):
    default_toolset = toolset
    default_toolset_version = version


pre_build_hook = []

def add_pre_build_hook(callable):
    pre_build_hook.append(callable)

post_build_hook = None

def set_post_build_hook(callable):
    post_build_hook = callable

################################################################################
#
# Local rules.
#
################################################################################

# Returns actual Jam targets to be used for executing a clean request.
#
def actual_clean_targets(targets):

    # Construct a list of projects explicitly detected as targets on this build
    # system run. These are the projects under which cleaning is allowed.
    for t in targets:
        if isinstance(t, b2.build.targets.ProjectTarget):
            project_targets.append(t.project_module())

    # Construct a list of targets explicitly detected on this build system run
    # as a result of building main targets.
    targets_to_clean = set()
    for t in results_of_main_targets:
        # Do not include roots or sources.
        targets_to_clean.update(virtual_target.traverse(t))

    to_clean = []
    for t in get_manager().virtual_targets().all_targets():

        # Remove only derived targets.
        if t.action():
            p = t.project()
            if t in targets_to_clean or should_clean_project(p.project_module()):
                to_clean.append(t)

    return [t.actualize() for t in to_clean]

_target_id_split = re.compile("(.*)//(.*)")

# Given a target id, try to find and return the corresponding target. This is
# only invoked when there is no Jamfile in ".". This code somewhat duplicates
# code in project-target.find but we can not reuse that code without a
# project-targets instance.
#
def find_target(target_id):

    projects = get_manager().projects()
    m = _target_id_split.match(target_id)
    if m:
        pm = projects.find(m.group(1), ".")
    else:
        pm = projects.find(target_id, ".")

    if pm:
        result = projects.target(pm)

    if m:
        result = result.find(m.group(2))

    return result

def initialize_config_module(module_name, location=None):

    get_manager().projects().initialize(module_name, location)

# Helper rule used to load configuration files. Loads the first configuration
# file with the given 'filename' at 'path' into module with name 'module-name'.
# Not finding the requested file may or may not be treated as an error depending
# on the must-find parameter. Returns a normalized path to the loaded
# configuration file or nothing if no file was loaded.
#
def load_config(module_name, filename, paths, must_find=False):

    if debug_config:
        print "notice: Searching  '%s' for '%s' configuration file '%s." \
              % (paths, module_name, filename)

    where = None
    for path in paths:
        t = os.path.join(path, filename)
        if os.path.exists(t):
            where = t
            break

    if where:
        where = os.path.realpath(where)

        if debug_config:
            print "notice: Loading '%s' configuration file '%s' from '%s'." \
                  % (module_name, filename, where)

        # Set source location so that path-constant in config files
        # with relative paths work. This is of most importance
        # for project-config.jam, but may be used in other
        # config files as well.
        attributes = get_manager().projects().attributes(module_name) ;
        attributes.set('source-location', os.path.dirname(where), True)
        get_manager().projects().load_standalone(module_name, where)

    else:
        msg = "Configuration file '%s' not found in '%s'." % (filename, path)
        if must_find:
            get_manager().errors()(msg)

        elif debug_config:
            print msg

    return where

# Loads all the configuration files used by Boost Build in the following order:
#
#   -- test-config --
#   Loaded only if specified on the command-line using the --test-config
# command-line parameter. It is ok for this file not to exist even if
# specified. If this configuration file is loaded, regular site and user
# configuration files will not be. If a relative path is specified, file is
# searched for in the current folder.
#
#   -- site-config --
#   Always named site-config.jam. Will only be found if located on the system
# root path (Windows), /etc (non-Windows), user's home folder or the Boost
# Build path, in that order. Not loaded in case the test-config configuration
# file is loaded or the --ignore-site-config command-line option is specified.
#
#   -- user-config --
#   Named user-config.jam by default or may be named explicitly using the
# --user-config command-line option or the BOOST_BUILD_USER_CONFIG environment
# variable. If named explicitly the file is looked for from the current working
# directory and if the default one is used then it is searched for in the
# user's home directory and the Boost Build path, in that order. Not loaded in
# case either the test-config configuration file is loaded or an empty file
# name is explicitly specified. If the file name has been given explicitly then
# the file must exist.
#
# Test configurations have been added primarily for use by Boost Build's
# internal unit testing system but may be used freely in other places as well.
#
def load_configuration_files():

    # Flag indicating that site configuration should not be loaded.
    ignore_site_config = "--ignore-site-config" in sys.argv

    initialize_config_module("test-config")
    test_config = None
    for a in sys.argv:
        m = re.match("--test-config=(.*)$", a)
        if m:
            test_config = b2.util.unquote(m.group(1))
            break

    if test_config:
        where = load_config("test-config", os.path.basename(test_config), [os.path.dirname(test_config)])
        if where:
            if debug_config:
                print "notice: Regular site and user configuration files will"
                print "notice: be ignored due to the test configuration being loaded."

    user_path = [os.path.expanduser("~")] + bjam.variable("BOOST_BUILD_PATH")
    site_path = ["/etc"] + user_path
    if os.name in ["nt"]:
        site_path = [os.getenv("SystemRoot")] + user_path

    if debug_config and not test_config and ignore_site_config:
        print "notice: Site configuration files will be ignored due to the"
        print "notice: --ignore-site-config command-line option."

    initialize_config_module("site-config")
    if not test_config and not ignore_site_config:
        load_config('site-config', 'site-config.jam', site_path)

    initialize_config_module('user-config')
    if not test_config:

        # Here, user_config has value of None if nothing is explicitly
        # specified, and value of '' if user explicitly does not want
        # to load any user config.
        user_config = None
        for a in sys.argv:
            m = re.match("--user-config=(.*)$", a)
            if m:
                user_config = m.group(1)
                break

        if user_config is None:
            user_config = os.getenv("BOOST_BUILD_USER_CONFIG")

        # Special handling for the case when the OS does not strip the quotes
        # around the file name, as is the case when using Cygwin bash.
        user_config = b2.util.unquote(user_config)
        explicitly_requested = user_config

        if user_config is None:
            user_config = "user-config.jam"

        if user_config:
            if explicitly_requested:

                user_config = os.path.abspath(user_config)

                if debug_config:
                    print "notice: Loading explicitly specified user configuration file:"
                    print "    " + user_config

                    load_config('user-config', os.path.basename(user_config), [os.path.dirname(user_config)], True)
            else:
                load_config('user-config', os.path.basename(user_config), user_path)
        else:
            if debug_config:
                print "notice: User configuration file loading explicitly disabled."

    # We look for project-config.jam from "." upward. I am not sure this is
    # 100% right decision, we might as well check for it only alongside the
    # Jamroot file. However:
    # - We need to load project-config.jam before Jamroot
    # - We probably need to load project-config.jam even if there is no Jamroot
    #   - e.g. to implement automake-style out-of-tree builds.
    if os.path.exists("project-config.jam"):
        file = ["project-config.jam"]
    else:
        file = b2.util.path.glob_in_parents(".", ["project-config.jam"])

    if file:
        initialize_config_module('project-config', os.path.dirname(file[0]))
        load_config('project-config', "project-config.jam", [os.path.dirname(file[0])], True)


# Autoconfigure toolsets based on any instances of --toolset=xx,yy,...zz or
# toolset=xx,yy,...zz in the command line. May return additional properties to
# be processed as if they had been specified by the user.
#
def process_explicit_toolset_requests():

    extra_properties = []

    option_toolsets = [e for option in b2.util.regex.transform(sys.argv, "^--toolset=(.*)$")
                       for e in option.split(',')]
    feature_toolsets = [e for option in b2.util.regex.transform(sys.argv, "^toolset=(.*)$")
                       for e in option.split(',')]

    for t in option_toolsets + feature_toolsets:

        # Parse toolset-version/properties.
        (toolset_version, toolset, version) = re.match("(([^-/]+)-?([^/]+)?)/?.*", t).groups()

        if debug_config:
            print "notice: [cmdline-cfg] Detected command-line request for '%s': toolset= %s version=%s" \
            % (toolset_version, toolset, version)

        # If the toolset is not known, configure it now.
        known = False
        if toolset in feature.values("toolset"):
            known = True

        if known and version and not feature.is_subvalue("toolset", toolset, "version", version):
            known = False
        # TODO: we should do 'using $(toolset)' in case no version has been
        # specified and there are no versions defined for the given toolset to
        # allow the toolset to configure its default version. For this we need
        # to know how to detect whether a given toolset has any versions
        # defined. An alternative would be to do this whenever version is not
        # specified but that would require that toolsets correctly handle the
        # case when their default version is configured multiple times which
        # should be checked for all existing toolsets first.

        if not known:

            if debug_config:
                print "notice: [cmdline-cfg] toolset '%s' not previously configured; attempting to auto-configure now" % toolset_version
            if version is not None:
               using(toolset, version)
            else:
               using(toolset)

        else:

            if debug_config:

                print "notice: [cmdline-cfg] toolset '%s' already configured" % toolset_version

        # Make sure we get an appropriate property into the build request in
        # case toolset has been specified using the "--toolset=..." command-line
        # option form.
        if not t in sys.argv and not t in feature_toolsets:

            if debug_config:
                print "notice: [cmdline-cfg] adding toolset=%s) to the build request." % t ;
            extra_properties += "toolset=%s" % t

    return extra_properties



# Returns 'true' if the given 'project' is equal to or is a (possibly indirect)
# child to any of the projects requested to be cleaned in this build system run.
# Returns 'false' otherwise. Expects the .project-targets list to have already
# been constructed.
#
@cached
def should_clean_project(project):

    if project in project_targets:
        return True
    else:

        parent = get_manager().projects().attribute(project, "parent-module")
        if parent and parent != "user-config":
            return should_clean_project(parent)
        else:
            return False

################################################################################
#
# main()
# ------
#
################################################################################

def main():

    sys.argv = bjam.variable("ARGV")

    # FIXME: document this option.
    if "--profiling" in sys.argv:
        import cProfile
        r = cProfile.runctx('main_real()', globals(), locals(), "stones.prof")

        import pstats
        stats = pstats.Stats("stones.prof")
        stats.strip_dirs()
        stats.sort_stats('time', 'calls')
        stats.print_callers(20)
        return r
    else:
        try:
            return main_real()
        except ExceptionWithUserContext, e:
            e.report()

def main_real():

    global debug_config, out_xml

    debug_config = "--debug-configuration" in sys.argv
    out_xml = any(re.match("^--out-xml=(.*)$", a) for a in sys.argv)

    engine = Engine()

    global_build_dir = option.get("build-dir")
    manager = Manager(engine, global_build_dir)

    import b2.build.configure as configure

    if "--version" in sys.argv:

        version.report()
        return

    # This module defines types and generator and what not,
    # and depends on manager's existence
    import b2.tools.builtin

    b2.tools.common.init(manager)

    load_configuration_files()

    # Load explicitly specified toolset modules.
    extra_properties = process_explicit_toolset_requests()

    # Load the actual project build script modules. We always load the project
    # in the current folder so 'use-project' directives have any chance of
    # being seen. Otherwise, we would not be able to refer to subprojects using
    # target ids.
    current_project = None
    projects = get_manager().projects()
    if projects.find(".", "."):
        current_project = projects.target(projects.load("."))

    # Load the default toolset module if no other has already been specified.
    if not feature.values("toolset"):

        dt = default_toolset
        dtv = None
        if default_toolset:
            dtv = default_toolset_version
        else:
            dt = "gcc"
            if os.name == 'nt':
                dt = "msvc"
            # FIXME:
            #else if [ os.name ] = MACOSX
            #{
            #    default-toolset = darwin ;
            #}

        print "warning: No toolsets are configured."
        print "warning: Configuring default toolset '%s'." % dt
        print "warning: If the default is wrong, your build may not work correctly."
        print "warning: Use the \"toolset=xxxxx\" option to override our guess."
        print "warning: For more configuration options, please consult"
        print "warning: http://boost.org/boost-build2/doc/html/bbv2/advanced/configuration.html"

        using(dt, dtv)

    # Parse command line for targets and properties. Note that this requires
    # that all project files already be loaded.
    (target_ids, properties) = build_request.from_command_line(sys.argv[1:] + extra_properties)

    # Expand properties specified on the command line into multiple property
    # sets consisting of all legal property combinations. Each expanded property
    # set will be used for a single build run. E.g. if multiple toolsets are
    # specified then requested targets will be built with each of them.
    if properties:
        expanded = build_request.expand_no_defaults(properties)
    else:
        expanded = [property_set.empty()]

    # Check that we actually found something to build.
    if not current_project and not target_ids:
        get_manager().errors()("no Jamfile in current directory found, and no target references specified.")
        # FIXME:
        # EXIT

    # Flags indicating that this build system run has been started in order to
    # clean existing instead of create new targets. Note that these are not the
    # final flag values as they may get changed later on due to some special
    # targets being specified on the command line.
    clean = "--clean" in sys.argv
    cleanall = "--clean-all" in sys.argv

    # List of explicitly requested files to build. Any target references read
    # from the command line parameter not recognized as one of the targets
    # defined in the loaded Jamfiles will be interpreted as an explicitly
    # requested file to build. If any such files are explicitly requested then
    # only those files and the targets they depend on will be built and they
    # will be searched for among targets that would have been built had there
    # been no explicitly requested files.
    explicitly_requested_files = []

    # List of Boost Build meta-targets, virtual-targets and actual Jam targets
    # constructed in this build system run.
    targets = []
    virtual_targets = []
    actual_targets = []

    explicitly_requested_files = []

    # Process each target specified on the command-line and convert it into
    # internal Boost Build target objects. Detect special clean target. If no
    # main Boost Build targets were explictly requested use the current project
    # as the target.
    for id in target_ids:
        if id == "clean":
            clean = 1
        else:
            t = None
            if current_project:
                t = current_project.find(id, no_error=1)
            else:
                t = find_target(id)

            if not t:
                print "notice: could not find main target '%s'" % id
                print "notice: assuming it's a name of file to create " ;
                explicitly_requested_files.append(id)
            else:
                targets.append(t)

    if not targets:
        targets = [projects.target(projects.module_name("."))]

    # FIXME: put this BACK.

    ## if [ option.get dump-generators : : true ]
    ## {
    ##     generators.dump ;
    ## }


    # We wish to put config.log in the build directory corresponding
    # to Jamroot, so that the location does not differ depending on
    # directory where we do build.  The amount of indirection necessary
    # here is scary.
    first_project = targets[0].project()
    first_project_root_location = first_project.get('project-root')
    first_project_root_module = manager.projects().load(first_project_root_location)
    first_project_root = manager.projects().target(first_project_root_module)
    first_build_build_dir = first_project_root.build_dir()
    configure.set_log_file(os.path.join(first_build_build_dir, "config.log"))

    virtual_targets = []

    global results_of_main_targets

    # Now that we have a set of targets to build and a set of property sets to
    # build the targets with, we can start the main build process by using each
    # property set to generate virtual targets from all of our listed targets
    # and any of their dependants.
    for p in expanded:
        manager.set_command_line_free_features(property_set.create(p.free()))

        for t in targets:
            try:
                g = t.generate(p)
                if not isinstance(t, ProjectTarget):
                    results_of_main_targets.extend(g.targets())
                virtual_targets.extend(g.targets())
            except ExceptionWithUserContext, e:
                e.report()
            except Exception:
                raise

    # Convert collected virtual targets into actual raw Jam targets.
    for t in virtual_targets:
        actual_targets.append(t.actualize())


     # FIXME: restore
##     # If XML data output has been requested prepare additional rules and targets
##     # so we can hook into Jam to collect build data while its building and have
##     # it trigger the final XML report generation after all the planned targets
##     # have been built.
##     if $(.out-xml)
##     {
##         # Get a qualified virtual target name.
##         rule full-target-name ( target )
##         {
##             local name = [ $(target).name ] ;
##             local project = [ $(target).project ] ;
##             local project-path = [ $(project).get location ] ;
##             return $(project-path)//$(name) ;
##         }

##         # Generate an XML file containing build statistics for each constituent.
##         #
##         rule out-xml ( xml-file : constituents * )
##         {
##             # Prepare valid XML header and footer with some basic info.
##             local nl = "
## " ;
##             local jam       = [ version.jam ] ;
##             local os        = [ modules.peek : OS OSPLAT JAMUNAME ] "" ;
##             local timestamp = [ modules.peek : JAMDATE ] ;
##             local cwd       = [ PWD ] ;
##             local command   = $(.sys.argv) ;
##             local bb-version = [ version.boost-build ] ;
##             .header on $(xml-file) =
##                 "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
##                 "$(nl)<build format=\"1.0\" version=\"$(bb-version)\">"
##                 "$(nl)  <jam version=\"$(jam:J=.)\" />"
##                 "$(nl)  <os name=\"$(os[1])\" platform=\"$(os[2])\"><![CDATA[$(os[3-]:J= )]]></os>"
##                 "$(nl)  <timestamp><![CDATA[$(timestamp)]]></timestamp>"
##                 "$(nl)  <directory><![CDATA[$(cwd)]]></directory>"
##                 "$(nl)  <command><![CDATA[\"$(command:J=\" \")\"]]></command>"
##                 ;
##             .footer on $(xml-file) =
##                 "$(nl)</build>" ;

##             # Generate the target dependency graph.
##             .contents on $(xml-file) +=
##                 "$(nl)  <targets>" ;
##             for local t in [ virtual-target.all-targets ]
##             {
##                 local action = [ $(t).action ] ;
##                 if $(action)
##                     # If a target has no action, it has no dependencies.
##                 {
##                     local name = [ full-target-name $(t) ] ;
##                     local sources = [ $(action).sources ] ;
##                     local dependencies ;
##                     for local s in $(sources)
##                     {
##                         dependencies += [ full-target-name $(s) ] ;
##                     }

##                     local path = [ $(t).path ] ;
##                     local jam-target = [ $(t).actual-name ] ;

##                     .contents on $(xml-file) +=
##                         "$(nl)    <target>"
##                         "$(nl)      <name><![CDATA[$(name)]]></name>"
##                         "$(nl)      <dependencies>"
##                         "$(nl)        <dependency><![CDATA[$(dependencies)]]></dependency>"
##                         "$(nl)      </dependencies>"
##                         "$(nl)      <path><![CDATA[$(path)]]></path>"
##                         "$(nl)      <jam-target><![CDATA[$(jam-target)]]></jam-target>"
##                         "$(nl)    </target>"
##                         ;
##                 }
##             }
##             .contents on $(xml-file) +=
##                 "$(nl)  </targets>" ;

##             # Build $(xml-file) after $(constituents). Do so even if a
##             # constituent action fails and regenerate the xml on every bjam run.
##             INCLUDES $(xml-file) : $(constituents) ;
##             ALWAYS $(xml-file) ;
##             __ACTION_RULE__ on $(xml-file) = build-system.out-xml.generate-action ;
##             out-xml.generate $(xml-file) ;
##         }

##         # The actual build actions are here; if we did this work in the actions
##         # clause we would have to form a valid command line containing the
##         # result of @(...) below (the name of the XML file).
##         #
##         rule out-xml.generate-action ( args * : xml-file
##             : command status start end user system : output ? )
##         {
##             local contents =
##                 [ on $(xml-file) return $(.header) $(.contents) $(.footer) ] ;
##             local f = @($(xml-file):E=$(contents)) ;
##         }

##         # Nothing to do here; the *real* actions happen in
##         # out-xml.generate-action.
##         actions quietly out-xml.generate { }

##         # Define the out-xml file target, which depends on all the targets so
##         # that it runs the collection after the targets have run.
##         out-xml $(.out-xml) : $(actual-targets) ;

##         # Set up a global __ACTION_RULE__ that records all the available
##         # statistics about each actual target in a variable "on" the --out-xml
##         # target.
##         #
##         rule out-xml.collect ( xml-file : target : command status start end user
##             system : output ? )
##         {
##             local nl = "
## " ;
##             # Open the action with some basic info.
##             .contents on $(xml-file) +=
##                 "$(nl)  <action status=\"$(status)\" start=\"$(start)\" end=\"$(end)\" user=\"$(user)\" system=\"$(system)\">" ;

##             # If we have an action object we can print out more detailed info.
##             local action = [ on $(target) return $(.action) ] ;
##             if $(action)
##             {
##                 local action-name    = [ $(action).action-name ] ;
##                 local action-sources = [ $(action).sources     ] ;
##                 local action-props   = [ $(action).properties  ] ;

##                 # The qualified name of the action which we created the target.
##                 .contents on $(xml-file) +=
##                     "$(nl)    <name><![CDATA[$(action-name)]]></name>" ;

##                 # The sources that made up the target.
##                 .contents on $(xml-file) +=
##                     "$(nl)    <sources>" ;
##                 for local source in $(action-sources)
##                 {
##                     local source-actual = [ $(source).actual-name ] ;
##                     .contents on $(xml-file) +=
##                         "$(nl)      <source><![CDATA[$(source-actual)]]></source>" ;
##                 }
##                 .contents on $(xml-file) +=
##                     "$(nl)    </sources>" ;

##                 # The properties that define the conditions under which the
##                 # target was built.
##                 .contents on $(xml-file) +=
##                     "$(nl)    <properties>" ;
##                 for local prop in [ $(action-props).raw ]
##                 {
##                     local prop-name = [ MATCH ^<(.*)>$ : $(prop:G) ] ;
##                     .contents on $(xml-file) +=
##                         "$(nl)      <property name=\"$(prop-name)\"><![CDATA[$(prop:G=)]]></property>" ;
##                 }
##                 .contents on $(xml-file) +=
##                     "$(nl)    </properties>" ;
##             }

##             local locate = [ on $(target) return $(LOCATE) ] ;
##             locate ?= "" ;
##             .contents on $(xml-file) +=
##                 "$(nl)    <jam-target><![CDATA[$(target)]]></jam-target>"
##                 "$(nl)    <path><![CDATA[$(target:G=:R=$(locate))]]></path>"
##                 "$(nl)    <command><![CDATA[$(command)]]></command>"
##                 "$(nl)    <output><![CDATA[$(output)]]></output>" ;
##             .contents on $(xml-file) +=
##                 "$(nl)  </action>" ;
##         }

##         # When no __ACTION_RULE__ is set "on" a target, the search falls back to
##         # the global module.
##         module
##         {
##             __ACTION_RULE__ = build-system.out-xml.collect
##                 [ modules.peek build-system : .out-xml ] ;
##         }

##         IMPORT
##             build-system :
##             out-xml.collect
##             out-xml.generate-action
##             : :
##             build-system.out-xml.collect
##             build-system.out-xml.generate-action
##             ;
##     }

    j = option.get("jobs")
    if j:
        bjam.call("set-variable", PARALLELISM, j)

    k = option.get("keep-going", "true", "true")
    if k in ["on", "yes", "true"]:
        bjam.call("set-variable", "KEEP_GOING", "1")
    elif k in ["off", "no", "false"]:
        bjam.call("set-variable", "KEEP_GOING", "0")
    else:
        print "error: Invalid value for the --keep-going option"
        sys.exit()

    # The 'all' pseudo target is not strictly needed expect in the case when we
    # use it below but people often assume they always have this target
    # available and do not declare it themselves before use which may cause
    # build failures with an error message about not being able to build the
    # 'all' target.
    bjam.call("NOTFILE", "all")

    # And now that all the actual raw Jam targets and all the dependencies
    # between them have been prepared all that is left is to tell Jam to update
    # those targets.
    if explicitly_requested_files:
        # Note that this case can not be joined with the regular one when only
        # exact Boost Build targets are requested as here we do not build those
        # requested targets but only use them to construct the dependency tree
        # needed to build the explicitly requested files.
        # FIXME: add $(.out-xml)
        bjam.call("UPDATE", ["<e>%s" % x for x in explicitly_requested_files])
    elif cleanall:
        bjam.call("UPDATE", "clean-all")
    elif clean:
        manager.engine().set_update_action("common.Clean", "clean",
                                           actual_clean_targets(targets))
        bjam.call("UPDATE", "clean")
    else:
        # FIXME:
        #configure.print-configure-checks-summary ;

        if pre_build_hook:
            for h in pre_build_hook:
                h()

        bjam.call("DEPENDS", "all", actual_targets)
        ok = bjam.call("UPDATE_NOW", "all") # FIXME: add out-xml
        if post_build_hook:
            post_build_hook(ok)
        # Prevent automatic update of the 'all' target, now that
        # we have explicitly updated what we wanted.
        bjam.call("UPDATE")

    if manager.errors().count() == 0:
        return ["ok"]
    else:
        return []
