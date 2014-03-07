#!/usr/bin/python

# Copyright 2012. Jurko Gospodnetic
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test Boost Jam parser's source line tracking & reporting.

import BoostBuild


def test_eof_in_string():
    t = BoostBuild.Tester(pass_toolset=False)
    t.write("file.jam", '\n\n\naaa = "\n\n\n\n\n\n')
    t.run_build_system(["-ffile.jam"], status=1)
    t.expect_output_lines('file.jam:4: unmatched " in string at keyword =')
    t.expect_output_lines("file.jam:4: syntax error at EOF")
    t.cleanup()


def test_error_missing_argument(eof):
    """
      This use case used to cause a missing argument error to be reported in
    module '(builtin)' in line -1 when the input file did not contain a
    trailing newline.

    """
    t = BoostBuild.Tester(pass_toolset=False)
    t.write("file.jam", """\
rule f ( param ) { }
f ;%s""" % __trailing_newline(eof))
    t.run_build_system(["-ffile.jam"], status=1)
    t.expect_output_lines("file.jam:2: in module scope")
    t.expect_output_lines("file.jam:1:see definition of rule 'f' being called")
    t.cleanup()


def test_error_syntax(eof):
    t = BoostBuild.Tester(pass_toolset=False)
    t.write("file.jam", "ECHO%s" % __trailing_newline(eof))
    t.run_build_system(["-ffile.jam"], status=1)
    t.expect_output_lines("file.jam:1: syntax error at EOF")
    t.cleanup()


def test_traceback():
    t = BoostBuild.Tester(pass_toolset=False)
    t.write("file.jam", """\
NOTFILE all ;
ECHO [ BACKTRACE ] ;""")
    t.run_build_system(["-ffile.jam"])
    t.expect_output_lines("file.jam 2  module scope")
    t.cleanup()


def __trailing_newline(eof):
    """
      Helper function returning an empty string or a newling character to
    append to the current output line depending on whether we want that line to
    be the last line in the file (eof == True) or not (eof == False).

    """
    if eof:
        return ""
    return "\n"


test_error_missing_argument(eof=False)
test_error_missing_argument(eof=True)
test_error_syntax(eof=False)
test_error_syntax(eof=True)
test_traceback()
test_eof_in_string()
