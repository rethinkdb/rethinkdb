# Status: ported, except for --out-xml
# Base revision: 64488
#
# Copyright 2005 Dave Abrahams
# Copyright 2002, 2003, 2004, 2005, 2010 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# This module implements regression testing framework. It declares a number of
# main target rules which perform some action and, if the results are OK,
# creates an output file.
#
# The exact list of rules is:
# 'compile'       -- creates .test file if compilation of sources was
#                    successful.
# 'compile-fail'  -- creates .test file if compilation of sources failed.
# 'run'           -- creates .test file is running of executable produced from
#                    sources was successful. Also leaves behind .output file
#                    with the output from program run.
# 'run-fail'      -- same as above, but .test file is created if running fails.
#
# In all cases, presence of .test file is an indication that the test passed.
# For more convenient reporting, you might want to use C++ Boost regression
# testing utilities (see http://www.boost.org/more/regression.html).
#
# For historical reason, a 'unit-test' rule is available which has the same
# syntax as 'exe' and behaves just like 'run'.

# Things to do:
#  - Teach compiler_status handle Jamfile.v2.
# Notes:
#  - <no-warn> is not implemented, since it is Como-specific, and it is not
#    clear how to implement it
#  - std::locale-support is not implemented (it is used in one test).

import b2.build.feature as feature
import b2.build.type as type
import b2.build.targets as targets
import b2.build.generators as generators
import b2.build.toolset as toolset
import b2.tools.common as common
import b2.util.option as option
import b2.build_system as build_system



from b2.manager import get_manager
from b2.util import stem, bjam_signature
from b2.util.sequence import unique

import bjam

import re
import os.path
import sys

def init():
    pass

# Feature controling the command used to lanch test programs.
feature.feature("testing.launcher", [], ["free", "optional"])

feature.feature("test-info", [], ["free", "incidental"])
feature.feature("testing.arg", [], ["free", "incidental"])
feature.feature("testing.input-file", [], ["free", "dependency"])

feature.feature("preserve-test-targets", ["on", "off"], ["incidental", "propagated"])

# Register target types.
type.register("TEST", ["test"])
type.register("COMPILE", [], "TEST")
type.register("COMPILE_FAIL", [], "TEST")

type.register("RUN_OUTPUT", ["run"])
type.register("RUN", [], "TEST")
type.register("RUN_FAIL", [], "TEST")

type.register("LINK", [], "TEST")
type.register("LINK_FAIL", [], "TEST")
type.register("UNIT_TEST", ["passed"], "TEST")

__all_tests = []

# Declare the rules which create main targets. While the 'type' module already
# creates rules with the same names for us, we need extra convenience: default
# name of main target, so write our own versions.

# Helper rule. Create a test target, using basename of first source if no target
# name is explicitly passed. Remembers the created target in a global variable.
def make_test(target_type, sources, requirements, target_name=None):

    if not target_name:
        target_name = stem(os.path.basename(sources[0]))

    # Having periods (".") in the target name is problematic because the typed
    # generator will strip the suffix and use the bare name for the file
    # targets. Even though the location-prefix averts problems most times it
    # does not prevent ambiguity issues when referring to the test targets. For
    # example when using the XML log output. So we rename the target to remove
    # the periods, and provide an alias for users.
    real_name = target_name.replace(".", "~")

    project = get_manager().projects().current()
    # The <location-prefix> forces the build system for generate paths in the
    # form '$build_dir/array1.test/gcc/debug'. This is necessary to allow
    # post-processing tools to work.
    t = get_manager().targets().create_typed_target(
        type.type_from_rule_name(target_type), project, real_name, sources,
        requirements + ["<location-prefix>" + real_name + ".test"], [], [])

    # The alias to the real target, per period replacement above.
    if real_name != target_name:
        get_manager().projects().project_rules().all_names_["alias"](
            target_name, [t])

    # Remember the test (for --dump-tests). A good way would be to collect all
    # given a project. This has some technical problems: e.g. we can not call
    # this dump from a Jamfile since projects referred by 'build-project' are
    # not available until the whole Jamfile has been loaded.
    __all_tests.append(t)
    return t


# Note: passing more that one cpp file here is known to fail. Passing a cpp file
# and a library target works.
#
@bjam_signature((["sources", "*"], ["requirements", "*"], ["target_name", "?"]))
def compile(sources, requirements, target_name=None):
    return make_test("compile", sources, requirements, target_name)

@bjam_signature((["sources", "*"], ["requirements", "*"], ["target_name", "?"]))
def compile_fail(sources, requirements, target_name=None):
    return make_test("compile-fail", sources, requirements, target_name)

@bjam_signature((["sources", "*"], ["requirements", "*"], ["target_name", "?"]))
def link(sources, requirements, target_name=None):
    return make_test("link", sources, requirements, target_name)

@bjam_signature((["sources", "*"], ["requirements", "*"], ["target_name", "?"]))
def link_fail(sources, requirements, target_name=None):
    return make_test("link-fail", sources, requirements, target_name)

def handle_input_files(input_files):
    if len(input_files) > 1:
        # Check that sorting made when creating property-set instance will not
        # change the ordering.
        if sorted(input_files) != input_files:
            get_manager().errors()("Names of input files must be sorted alphabetically\n" +
                                   "due to internal limitations")
    return ["<testing.input-file>" + f for f in input_files]

@bjam_signature((["sources", "*"], ["args", "*"], ["input_files", "*"],
                 ["requirements", "*"], ["target_name", "?"],
                 ["default_build", "*"]))                 
def run(sources, args, input_files, requirements, target_name=None, default_build=[]):
    if args:
        requirements.append("<testing.arg>" + " ".join(args))
    requirements.extend(handle_input_files(input_files))
    return make_test("run", sources, requirements, target_name)

@bjam_signature((["sources", "*"], ["args", "*"], ["input_files", "*"],
                 ["requirements", "*"], ["target_name", "?"],
                 ["default_build", "*"]))                 
def run_fail(sources, args, input_files, requirements, target_name=None, default_build=[]):
    if args:
        requirements.append("<testing.arg>" + " ".join(args))
    requirements.extend(handle_input_files(input_files))
    return make_test("run-fail", sources, requirements, target_name)

# Register all the rules
for name in ["compile", "compile-fail", "link", "link-fail", "run", "run-fail"]:
    get_manager().projects().add_rule(name, getattr(sys.modules[__name__], name.replace("-", "_")))

# Use 'test-suite' as a synonym for 'alias', for backward compatibility.
from b2.build.alias import alias
get_manager().projects().add_rule("test-suite", alias)

# For all main targets in 'project-module', which are typed targets with type
# derived from 'TEST', produce some interesting information.
#
def dump_tests():
    for t in __all_tests:
        dump_test(t)

# Given a project location in normalized form (slashes are forward), compute the
# name of the Boost library.
#
__ln1 = re.compile("/(tools|libs)/(.*)/(test|example)")
__ln2 = re.compile("/(tools|libs)/(.*)$")
__ln3 = re.compile("(/status$)")
def get_library_name(path):
    
    path = path.replace("\\", "/")
    match1 = __ln1.match(path)
    match2 = __ln2.match(path)
    match3 = __ln3.match(path)

    if match1:
        return match1.group(2)
    elif match2:
        return match2.group(2)
    elif match3:
        return ""
    elif option.get("dump-tests", False, True):
        # The 'run' rule and others might be used outside boost. In that case,
        # just return the path, since the 'library name' makes no sense.
        return path

# Was an XML dump requested?
__out_xml = option.get("out-xml", False, True)

# Takes a target (instance of 'basic-target') and prints
#   - its type
#   - its name
#   - comments specified via the <test-info> property
#   - relative location of all source from the project root.
#
def dump_test(target):
    type = target.type()
    name = target.name()
    project = target.project()

    project_root = project.get('project-root')
    library = get_library_name(os.path.abspath(project.get('location')))
    if library:
        name = library + "/" + name

    sources = target.sources()
    source_files = []
    for s in sources:
        if isinstance(s, targets.FileReference):
            location = os.path.abspath(os.path.join(s.location(), s.name()))
            source_files.append(os.path.relpath(location, os.path.abspath(project_root)))

    target_name = project.get('location') + "//" + target.name() + ".test"

    test_info = target.requirements().get('test-info')
    test_info = " ".join('"' + ti + '"' for ti in test_info)

    # If the user requested XML output on the command-line, add the test info to
    # that XML file rather than dumping them to stdout.
    #if $(.out-xml)
    #{
#        local nl = "
#" ;
#        .contents on $(.out-xml) +=
#            "$(nl)  <test type=\"$(type)\" name=\"$(name)\">"
#            "$(nl)    <target><![CDATA[$(target-name)]]></target>"
#            "$(nl)    <info><![CDATA[$(test-info)]]></info>"
#            "$(nl)    <source><![CDATA[$(source-files)]]></source>"
#            "$(nl)  </test>"
#            ;
#    }
#    else

    source_files = " ".join('"' + s + '"' for s in source_files)
    if test_info:
        print 'boost-test(%s) "%s" [%s] : %s' % (type, name, test_info, source_files)
    else:
        print 'boost-test(%s) "%s" : %s' % (type, name, source_files)

# Register generators. Depending on target type, either 'expect-success' or
# 'expect-failure' rule will be used.
generators.register_standard("testing.expect-success", ["OBJ"], ["COMPILE"])
generators.register_standard("testing.expect-failure", ["OBJ"], ["COMPILE_FAIL"])
generators.register_standard("testing.expect-success", ["RUN_OUTPUT"], ["RUN"])
generators.register_standard("testing.expect-failure", ["RUN_OUTPUT"], ["RUN_FAIL"])
generators.register_standard("testing.expect-success", ["EXE"], ["LINK"])
generators.register_standard("testing.expect-failure", ["EXE"], ["LINK_FAIL"])

# Generator which runs an EXE and captures output.
generators.register_standard("testing.capture-output", ["EXE"], ["RUN_OUTPUT"])

# Generator which creates a target if sources run successfully. Differs from RUN
# in that run output is not captured. The reason why it exists is that the 'run'
# rule is much better for automated testing, but is not user-friendly (see
# http://article.gmane.org/gmane.comp.lib.boost.build/6353).
generators.register_standard("testing.unit-test", ["EXE"], ["UNIT_TEST"])

# FIXME: if those calls are after bjam.call, then bjam will crash
# when toolset.flags calls bjam.caller.
toolset.flags("testing.capture-output", "ARGS", [], ["<testing.arg>"])
toolset.flags("testing.capture-output", "INPUT_FILES", [], ["<testing.input-file>"])
toolset.flags("testing.capture-output", "LAUNCHER", [], ["<testing.launcher>"])

toolset.flags("testing.unit-test", "LAUNCHER", [], ["<testing.launcher>"])
toolset.flags("testing.unit-test", "ARGS", [], ["<testing.arg>"])

# This is a composing generator to support cases where a generator for the
# specified target constructs other targets as well. One such example is msvc's
# exe generator that constructs both EXE and PDB targets.
type.register("TIME", ["time"])
generators.register_composing("testing.time", [], ["TIME"])


# The following code sets up actions for this module. It's pretty convoluted,
# but the basic points is that we most of actions are defined by Jam code
# contained in testing-aux.jam, which we load into Jam module named 'testing'

def run_path_setup(target, sources, ps):

    # For testing, we need to make sure that all dynamic libraries needed by the
    # test are found. So, we collect all paths from dependency libraries (via
    # xdll-path property) and add whatever explicit dll-path user has specified.
    # The resulting paths are added to the environment on each test invocation.
    dll_paths = ps.get('dll-path')
    dll_paths.extend(ps.get('xdll-path'))
    dll_paths.extend(bjam.call("get-target-variable", sources, "RUN_PATH"))
    dll_paths = unique(dll_paths)
    if dll_paths:
        bjam.call("set-target-variable", target, "PATH_SETUP",
                  common.prepend_path_variable_command(
                     common.shared_library_path_variable(), dll_paths))

def capture_output_setup(target, sources, ps):
    run_path_setup(target, sources, ps)

    if ps.get('preserve-test-targets') == ['off']:
        bjam.call("set-target-variable", target, "REMOVE_TEST_TARGETS", "1")

get_manager().engine().register_bjam_action("testing.capture-output",
                                            capture_output_setup)


path = os.path.dirname(get_manager().projects().loaded_tool_module_path_[__name__])
import b2.util.os_j
get_manager().projects().project_rules()._import_rule("testing", "os.name",
                                                      b2.util.os_j.name)
import b2.tools.common
get_manager().projects().project_rules()._import_rule("testing", "common.rm-command",
                                                      b2.tools.common.rm_command)
get_manager().projects().project_rules()._import_rule("testing", "common.file-creation-command",
                                                      b2.tools.common.file_creation_command)

bjam.call("load", "testing", os.path.join(path, "testing-aux.jam"))


for name in ["expect-success", "expect-failure", "time"]:
    get_manager().engine().register_bjam_action("testing." + name)

get_manager().engine().register_bjam_action("testing.unit-test",
                                            run_path_setup)

if option.get("dump-tests", False, True):
    build_system.add_pre_build_hook(dump_tests)
