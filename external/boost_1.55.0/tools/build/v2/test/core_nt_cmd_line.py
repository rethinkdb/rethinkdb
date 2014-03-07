#!/usr/bin/python

# Copyright 2001 Dave Abrahams
# Copyright 2011 Steven Watanabe
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Tests Windows command line construction.
#
# Note that the regular 'echo' is an internal shell command on Windows and
# therefore can not be called directly as a standalone Windows process.

import BoostBuild
import os
import re
import sys


executable = sys.executable.replace("\\", "/")
if " " in executable:
    executable = '"%s"' % executable


def string_of_length(n):
    if n <= 0:
        return ""
    n -= 1
    y = ['', '$(1x10-1)', '$(10x10-1)', '$(100x10-1)', '$(1000x10-1)']
    result = []
    for i in reversed(xrange(5)):
        x, n = divmod(n, 10 ** i)
        result += [y[i]] * x
    result.append('x')
    return " ".join(result)


#   Boost Jam currently does not allow preparing actions with completely empty
# content and always requires at least a single whitespace after the opening
# brace in order to satisfy its Boost Jam language grammar rules.
def test_raw_empty():
    whitespace_in = "  \n\n\r\r\v\v\t\t   \t   \r\r   \n\n"

    #   We tell the testing system to read its child process output as raw
    # binary data but the bjam process we run will read its input file and
    # write out its output as text, i.e. convert all of our "\r\n" sequences to
    # "\n" on input and all of its "\n" characters back to "\r\n" on output.
    # This means that any lone "\n" input characters not preceded by "\r" will
    # get an extra "\r" added in front of it on output.
    whitespace_out = whitespace_in.replace("\r\n", "\n").replace("\n", "\r\n")

    t = BoostBuild.Tester(["-d2", "-d+4"], pass_d0=False, pass_toolset=0,
        use_test_config=False)
    t.write("file.jam", """\
actions do_empty {%s}
JAMSHELL = %% ;
do_empty all ;
""" % (whitespace_in))
    t.run_build_system(["-ffile.jam"], universal_newlines=False)
    t.expect_output_lines("do_empty all")
    t.expect_output_lines("Executing raw command directly", False)
    if "\r\n%s\r\n" % whitespace_out not in t.stdout():
        BoostBuild.annotation("failure", "Whitespace action content not found "
            "on stdout.")
        t.fail_test(1, dump_difference=False)
    t.cleanup()


def test_raw_nt(n=None, error=False):
    t = BoostBuild.Tester(["-d1", "-d+4"], pass_d0=False, pass_toolset=0,
        use_test_config=False)

    cmd_prefix = "%s -c \"print('XXX: " % executable
    cmd_suffix = "')\""
    cmd_extra_length = len(cmd_prefix) + len(cmd_suffix)

    if n == None:
        n = cmd_extra_length

    data_length = n - cmd_extra_length
    if data_length < 0:
        BoostBuild.annotation("failure", """\
Can not construct Windows command of desired length. Requested command length
too short for the current test configuration.
    Requested command length: %d
    Minimal supported command length: %d
""" % (n, cmd_extra_length))
        t.fail_test(1, dump_difference=False)

    #   Each $(Xx10-1) variable contains X words of 9 characters each, which,
    # including spaces between words, brings the total number of characters in
    # its string representation to X * 10 - 1 (X * 9 characters + (X - 1)
    # spaces).
    t.write("file.jam", """\
ten = 0 1 2 3 4 5 6 7 8 9 ;

1x10-1 = 123456789 ;
10x10-1 = $(ten)12345678 ;
100x10-1 = $(ten)$(ten)1234567 ;
1000x10-1 = $(ten)$(ten)$(ten)123456 ;

actions do_echo
{
    %s%s%s
}
JAMSHELL = %% ;
do_echo all ;
""" % (cmd_prefix, string_of_length(data_length), cmd_suffix))
    if error:
        expected_status = 1
    else:
        expected_status = 0
    t.run_build_system(["-ffile.jam"], status=expected_status)
    if error:
        t.expect_output_lines("Executing raw command directly", False)
        t.expect_output_lines("do_echo action is too long (%d, max 32766):" % n
            )
        t.expect_output_lines("XXX: *", False)
    else:
        t.expect_output_lines("Executing raw command directly")
        t.expect_output_lines("do_echo action is too long*", False)

        m = re.search("^XXX: (.*)$", t.stdout(), re.MULTILINE)
        if not m:
            BoostBuild.annotation("failure", "Expected output line starting "
                "with 'XXX: ' not found.")
            t.fail_test(1, dump_difference=False)
        if len(m.group(1)) != data_length:
            BoostBuild.annotation("failure", """Unexpected output data length.
    Expected: %d
    Received: %d""" % (n, len(m.group(1))))
            t.fail_test(1, dump_difference=False)

    t.cleanup()


def test_raw_to_shell_fallback_nt():
    t = BoostBuild.Tester(["-d1", "-d+4"], pass_d0=False, pass_toolset=0,
        use_test_config=False)

    cmd_prefix = '%s -c print(' % executable
    cmd_suffix = ')'

    t.write("file_multiline.jam", """\
actions do_multiline
{
    echo one


    echo two
}
JAMSHELL = % ;
do_multiline all ;
""")
    t.run_build_system(["-ffile_multiline.jam"])
    t.expect_output_lines("do_multiline all")
    t.expect_output_lines("one")
    t.expect_output_lines("two")
    t.expect_output_lines("Executing raw command directly", False)
    t.expect_output_lines("Executing using a command file and the shell: "
        "cmd.exe /Q/C")

    t.write("file_redirect.jam", """\
actions do_redirect { echo one > two.txt }
JAMSHELL = % ;
do_redirect all ;
""")
    t.run_build_system(["-ffile_redirect.jam"])
    t.expect_output_lines("do_redirect all")
    t.expect_output_lines("one", False)
    t.expect_output_lines("Executing raw command directly", False)
    t.expect_output_lines("Executing using a command file and the shell: "
        "cmd.exe /Q/C")
    t.expect_addition("two.txt")

    t.write("file_pipe.jam", """\
actions do_pipe
{
    echo one | echo two
}
JAMSHELL = % ;
do_pipe all ;
""")
    t.run_build_system(["-ffile_pipe.jam"])
    t.expect_output_lines("do_pipe all")
    t.expect_output_lines("one*", False)
    t.expect_output_lines("two")
    t.expect_output_lines("Executing raw command directly", False)
    t.expect_output_lines("Executing using a command file and the shell: "
        "cmd.exe /Q/C")

    t.write("file_single_quoted.jam", """\
actions do_single_quoted { %s'5>10'%s }
JAMSHELL = %% ;
do_single_quoted all ;
""" % (cmd_prefix, cmd_suffix))
    t.run_build_system(["-ffile_single_quoted.jam"])
    t.expect_output_lines("do_single_quoted all")
    t.expect_output_lines("5>10")
    t.expect_output_lines("Executing raw command directly")
    t.expect_output_lines("Executing using a command file and the shell: "
        "cmd.exe /Q/C", False)
    t.expect_nothing_more()

    t.write("file_double_quoted.jam", """\
actions do_double_quoted { %s"5>10"%s }
JAMSHELL = %% ;
do_double_quoted all ;
""" % (cmd_prefix, cmd_suffix))
    t.run_build_system(["-ffile_double_quoted.jam"])
    t.expect_output_lines("do_double_quoted all")
    # The difference between this example and the similar previous one using
    # single instead of double quotes stems from how the used Python executable
    # parses the command-line string received from Windows.
    t.expect_output_lines("False")
    t.expect_output_lines("Executing raw command directly")
    t.expect_output_lines("Executing using a command file and the shell: "
        "cmd.exe /Q/C", False)
    t.expect_nothing_more()

    t.write("file_escaped_quote.jam", """\
actions do_escaped_quote { %s\\"5>10\\"%s }
JAMSHELL = %% ;
do_escaped_quote all ;
""" % (cmd_prefix, cmd_suffix))
    t.run_build_system(["-ffile_escaped_quote.jam"])
    t.expect_output_lines("do_escaped_quote all")
    t.expect_output_lines("5>10")
    t.expect_output_lines("Executing raw command directly", False)
    t.expect_output_lines("Executing using a command file and the shell: "
        "cmd.exe /Q/C")
    t.expect_nothing_more()

    t.cleanup()


###############################################################################
#
# main()
# ------
#
###############################################################################

if os.name == 'nt':
    test_raw_empty()

    #   Can not test much shorter lengths as the shortest possible command line
    # line length constructed in this depends on the runtime environment, e.g.
    # path to the Panther executable running this test.
    test_raw_nt()
    test_raw_nt(255)
    test_raw_nt(1000)
    test_raw_nt(8000)
    test_raw_nt(8191)
    test_raw_nt(8192)
    test_raw_nt(10000)
    test_raw_nt(30000)
    test_raw_nt(32766)
    #   CreateProcessA() Windows API places a limit of 32768 on the allowed
    # command-line length, including a trailing Unicode (2-byte) nul-terminator
    # character.
    test_raw_nt(32767, error=True)
    test_raw_nt(40000, error=True)
    test_raw_nt(100001, error=True)

    test_raw_to_shell_fallback_nt()