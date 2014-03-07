#!/usr/bin/python

# Copyright 2002-2005 Dave Abrahams.
# Copyright 2002-2006 Vladimir Prus.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#         http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

import os
import os.path
import sys

xml = "--xml" in sys.argv
toolset = BoostBuild.get_toolset()


# Clear environment for testing.
#
for s in ("BOOST_ROOT", "BOOST_BUILD_PATH", "JAM_TOOLSET", "BCCROOT",
    "MSVCDir", "MSVC", "MSVCNT", "MINGW", "watcom"):
    try:
        del os.environ[s]
    except:
        pass

BoostBuild.set_defer_annotations(1)


def run_tests(critical_tests, other_tests):
    """
      Runs first the critical_tests and then the other_tests.

      Writes the name of the first failed test to test_results.txt. Critical
    tests are run in the specified order, other tests are run starting with the
    one that failed first on the last test run.

    """
    last_failed = last_failed_test()
    other_tests = reorder_tests(other_tests, last_failed)
    all_tests = critical_tests + other_tests

    invocation_dir = os.getcwd()
    max_test_name_len = 10
    for x in all_tests:
        if len(x) > max_test_name_len:
            max_test_name_len = len(x)

    pass_count = 0
    failures_count = 0

    for test in all_tests:
        if not xml:
            print("%%-%ds :" % max_test_name_len % test),

        passed = 0
        try:
            __import__(test)
            passed = 1
        except KeyboardInterrupt:
            """This allows us to abort the testing manually using Ctrl-C."""
            raise
        except SystemExit:
            """This is the regular way our test scripts are supposed to report
            test failures."""
        except:
            exc_type, exc_value, exc_tb = sys.exc_info()
            try:
                BoostBuild.annotation("failure - unhandled exception", "%s - "
                    "%s" % (exc_type.__name__, exc_value))
                BoostBuild.annotate_stack_trace(exc_tb)
            finally:
                #   Explicitly clear a hard-to-garbage-collect traceback
                # related reference cycle as per documented sys.exc_info()
                # usage suggestion.
                del exc_tb

        if passed:
            pass_count += 1
        else:
            failures_count += 1
            if failures_count == 1:
                f = open(os.path.join(invocation_dir, "test_results.txt"), "w")
                try:
                    f.write(test)
                finally:
                    f.close()

        #   Restore the current directory, which might have been changed by the
        # test.
        os.chdir(invocation_dir)

        if not xml:
            if passed:
                print("PASSED")
            else:
                print("FAILED")
        else:
            rs = "succeed"
            if not passed:
                rs = "fail"
            print """
<test-log library="build" test-name="%s" test-type="run" toolset="%s" test-program="%s" target-directory="%s">
<run result="%s">""" % (test, toolset, "tools/build/v2/test/" + test + ".py",
                "boost/bin.v2/boost.build.tests/" + toolset + "/" + test, rs)
            if not passed:
                BoostBuild.flush_annotations(1)
            print """
</run>
</test-log>
"""
        sys.stdout.flush()  # Makes testing under emacs more entertaining.
        BoostBuild.clear_annotations()

    # Erase the file on success.
    if failures_count == 0:
        open("test_results.txt", "w").close()

    if not xml:
        print """
        === Test summary ===
        PASS: %d
        FAIL: %d
        """ % (pass_count, failures_count)


def last_failed_test():
    "Returns the name of the last failed test or None."
    try:
        f = open("test_results.txt")
        try:
            return f.read().strip()
        finally:
            f.close()
    except Exception:
        return None


def reorder_tests(tests, first_test):
    try:
        n = tests.index(first_test)
        return [first_test] + tests[:n] + tests[n + 1:]
    except ValueError:
        return tests


critical_tests = ["unit_tests", "module_actions", "startup_v2", "core_d12",
    "core_typecheck", "core_delete_module", "core_language", "core_arguments",
    "core_varnames", "core_import_module"]

# We want to collect debug information about the test site before running any
# of the tests, but only when not running the tests interactively. Then the
# user can easily run this always-failing test directly to see what it would
# have returned and there is no need to have it spoil a possible 'all tests
# passed' result.
if xml:
    critical_tests.insert(0, "collect_debug_info")

tests = ["absolute_sources",
         "alias",
         "alternatives",
         "bad_dirname",
         "build_dir",
         "build_file",
         "build_no",
         "builtin_echo",
         "builtin_exit",
         "builtin_split_by_characters",
         "c_file",
         "chain",
         "clean",
         "composite",
         "conditionals",
         "conditionals2",
         "conditionals3",
         "conditionals_multiple",
         "configuration",
         "copy_time",
         "core_action_output",
         "core_action_status",
         "core_actions_quietly",
         "core_at_file",
         "core_bindrule",
         "core_nt_cmd_line",
         "core_option_d2",
         "core_option_l",
         "core_option_n",
         "core_parallel_actions",
         "core_parallel_multifile_actions_1",
         "core_parallel_multifile_actions_2",
         "core_source_line_tracking",
         "core_update_now",
         "core_variables_in_actions",
         "custom_generator",
         "default_build",
         "default_features",
# This test is known to be broken itself.
#         "default_toolset",
         "dependency_property",
         "dependency_test",
         "direct_request_test",
         "disambiguation",
         "dll_path",
         "double_loading",
         "duplicate",
         "example_libraries",
         "example_make",
         "exit_status",
         "expansion",
         "explicit",
         "free_features_request",
         "generator_selection",
         "generators_test",
         "implicit_dependency",
         "indirect_conditional",
         "inherit_toolset",
         "inherited_dependency",
         "inline",
         "lib_source_property",
         "library_chain",
         "library_property",
         "load_order",
         "loop",
         "make_rule",
         "message",
         "ndebug",
         "no_type",
         "notfile",
         "ordered_include",
         "out_of_tree",
         "path_features",
         "prebuilt",
         "print",
         "project_dependencies",
         "project_glob",
         "project_id",
         "project_root_constants",
         "project_root_rule",
         "project_test3",
         "project_test4",
         "property_expansion",
         "rebuilds",
         "regression",
         "relative_sources",
         "remove_requirement",
         "rescan_header",
         "resolution",
         "scanner_causing_rebuilds",
         "searched_lib",
         "skipping",
         "sort_rule",
         "source_locations",
         "space_in_path",
         "stage",
         "standalone",
         "static_and_shared_library",
         "suffix",
         "tag",
         "test_result_dumping",
         "test_rc",
         "testing_support",
         "timedata",
         "unit_test",
         "unused",
         "use_requirements",
         "using",
         "wrapper",
         "wrong_project",
         "zlib"
         ]

if os.name == "posix":
    tests.append("symlink")
    # On Windows, library order is not important, so skip this test. Besides,
    # it fails ;-). Further, the test relies on the fact that on Linux, one can
    # build a shared library with unresolved symbols. This is not true on
    # Windows, even with cygwin gcc.
    if "CYGWIN" not in os.uname()[0]:
        tests.append("library_order")

if toolset.startswith("gcc"):
    tests.append("gcc_runtime")

if toolset.startswith("gcc") or toolset.startswith("msvc"):
    tests.append("pch")

if "--extras" in sys.argv:
    tests.append("boostbook")
    tests.append("qt4")
    tests.append("qt5")
    tests.append("example_qt4")
    # Requires ./whatever.py to work, so is not guaranted to work everywhere.
    tests.append("example_customization")
    # Requires gettext tools.
    tests.append("example_gettext")
elif not xml:
    print("Note: skipping extra tests")

run_tests(critical_tests, tests)
