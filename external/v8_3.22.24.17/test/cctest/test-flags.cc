// Copyright 2006-2008 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdlib.h>

#include "v8.h"
#include "cctest.h"

using namespace v8::internal;

// This test must be executed first!
TEST(Default) {
  CHECK(FLAG_testing_bool_flag);
  CHECK_EQ(13, FLAG_testing_int_flag);
  CHECK_EQ(2.5, FLAG_testing_float_flag);
  CHECK_EQ(0, strcmp(FLAG_testing_string_flag, "Hello, world!"));
}


static void SetFlagsToDefault() {
  FlagList::ResetAllFlags();
  TestDefault();
}


TEST(Flags1) {
  FlagList::PrintHelp();
}


TEST(Flags2) {
  SetFlagsToDefault();
  int argc = 8;
  const char* argv[] = { "Test2", "-notesting-bool-flag",
                         "--notesting-maybe-bool-flag", "notaflag",
                         "--testing_int_flag=77", "-testing_float_flag=.25",
                         "--testing_string_flag", "no way!" };
  CHECK_EQ(0, FlagList::SetFlagsFromCommandLine(&argc,
                                                const_cast<char **>(argv),
                                                false));
  CHECK_EQ(8, argc);
  CHECK(!FLAG_testing_bool_flag);
  CHECK(FLAG_testing_maybe_bool_flag.has_value);
  CHECK(!FLAG_testing_maybe_bool_flag.value);
  CHECK_EQ(77, FLAG_testing_int_flag);
  CHECK_EQ(.25, FLAG_testing_float_flag);
  CHECK_EQ(0, strcmp(FLAG_testing_string_flag, "no way!"));
}


TEST(Flags2b) {
  SetFlagsToDefault();
  const char* str =
      " -notesting-bool-flag notaflag   --testing_int_flag=77 "
      "-notesting-maybe-bool-flag   "
      "-testing_float_flag=.25  "
      "--testing_string_flag   no_way!  ";
  CHECK_EQ(0, FlagList::SetFlagsFromString(str, StrLength(str)));
  CHECK(!FLAG_testing_bool_flag);
  CHECK(FLAG_testing_maybe_bool_flag.has_value);
  CHECK(!FLAG_testing_maybe_bool_flag.value);
  CHECK_EQ(77, FLAG_testing_int_flag);
  CHECK_EQ(.25, FLAG_testing_float_flag);
  CHECK_EQ(0, strcmp(FLAG_testing_string_flag, "no_way!"));
}


TEST(Flags3) {
  SetFlagsToDefault();
  int argc = 9;
  const char* argv[] =
      { "Test3", "--testing_bool_flag", "--testing-maybe-bool-flag", "notaflag",
        "--testing_int_flag", "-666",
        "--testing_float_flag", "-12E10", "-testing-string-flag=foo-bar" };
  CHECK_EQ(0, FlagList::SetFlagsFromCommandLine(&argc,
                                                const_cast<char **>(argv),
                                                true));
  CHECK_EQ(2, argc);
  CHECK(FLAG_testing_bool_flag);
  CHECK(FLAG_testing_maybe_bool_flag.has_value);
  CHECK(FLAG_testing_maybe_bool_flag.value);
  CHECK_EQ(-666, FLAG_testing_int_flag);
  CHECK_EQ(-12E10, FLAG_testing_float_flag);
  CHECK_EQ(0, strcmp(FLAG_testing_string_flag, "foo-bar"));
}


TEST(Flags3b) {
  SetFlagsToDefault();
  const char* str =
      "--testing_bool_flag --testing-maybe-bool-flag notaflag "
      "--testing_int_flag -666 "
      "--testing_float_flag -12E10 "
      "-testing-string-flag=foo-bar";
  CHECK_EQ(0, FlagList::SetFlagsFromString(str, StrLength(str)));
  CHECK(FLAG_testing_bool_flag);
  CHECK(FLAG_testing_maybe_bool_flag.has_value);
  CHECK(FLAG_testing_maybe_bool_flag.value);
  CHECK_EQ(-666, FLAG_testing_int_flag);
  CHECK_EQ(-12E10, FLAG_testing_float_flag);
  CHECK_EQ(0, strcmp(FLAG_testing_string_flag, "foo-bar"));
}


TEST(Flags4) {
  SetFlagsToDefault();
  int argc = 3;
  const char* argv[] = { "Test4", "--testing_bool_flag", "--foo" };
  CHECK_EQ(0, FlagList::SetFlagsFromCommandLine(&argc,
                                                const_cast<char **>(argv),
                                                true));
  CHECK_EQ(2, argc);
  CHECK(!FLAG_testing_maybe_bool_flag.has_value);
}


TEST(Flags4b) {
  SetFlagsToDefault();
  const char* str = "--testing_bool_flag --foo";
  CHECK_EQ(2, FlagList::SetFlagsFromString(str, StrLength(str)));
  CHECK(!FLAG_testing_maybe_bool_flag.has_value);
}


TEST(Flags5) {
  SetFlagsToDefault();
  int argc = 2;
  const char* argv[] = { "Test5", "--testing_int_flag=\"foobar\"" };
  CHECK_EQ(1, FlagList::SetFlagsFromCommandLine(&argc,
                                                const_cast<char **>(argv),
                                                true));
  CHECK_EQ(2, argc);
}


TEST(Flags5b) {
  SetFlagsToDefault();
  const char* str = "                     --testing_int_flag=\"foobar\"";
  CHECK_EQ(1, FlagList::SetFlagsFromString(str, StrLength(str)));
}


TEST(Flags6) {
  SetFlagsToDefault();
  int argc = 4;
  const char* argv[] = { "Test5", "--testing-int-flag", "0",
                         "--testing_float_flag" };
  CHECK_EQ(3, FlagList::SetFlagsFromCommandLine(&argc,
                                                const_cast<char **>(argv),
                                                true));
  CHECK_EQ(2, argc);
}


TEST(Flags6b) {
  SetFlagsToDefault();
  const char* str = "       --testing-int-flag 0      --testing_float_flag    ";
  CHECK_EQ(3, FlagList::SetFlagsFromString(str, StrLength(str)));
}


TEST(FlagsJSArguments1) {
  SetFlagsToDefault();
  int argc = 6;
  const char* argv[] = {"TestJSArgs1",
                        "--testing-int-flag", "42",
                        "--", "testing-float-flag", "7"};
  CHECK_EQ(0, FlagList::SetFlagsFromCommandLine(&argc,
                                                const_cast<char **>(argv),
                                                true));
  CHECK_EQ(42, FLAG_testing_int_flag);
  CHECK_EQ(2.5, FLAG_testing_float_flag);
  CHECK_EQ(2, FLAG_js_arguments.argc);
  CHECK_EQ(0, strcmp(FLAG_js_arguments[0], "testing-float-flag"));
  CHECK_EQ(0, strcmp(FLAG_js_arguments[1], "7"));
  CHECK_EQ(1, argc);
}


TEST(FlagsJSArguments1b) {
  SetFlagsToDefault();
  const char* str = "--testing-int-flag 42 -- testing-float-flag 7";
  CHECK_EQ(0, FlagList::SetFlagsFromString(str, StrLength(str)));
  CHECK_EQ(42, FLAG_testing_int_flag);
  CHECK_EQ(2.5, FLAG_testing_float_flag);
  CHECK_EQ(2, FLAG_js_arguments.argc);
  CHECK_EQ(0, strcmp(FLAG_js_arguments[0], "testing-float-flag"));
  CHECK_EQ(0, strcmp(FLAG_js_arguments[1], "7"));
}


TEST(FlagsJSArguments2) {
  SetFlagsToDefault();
  const char* str = "--testing-int-flag 42 --js-arguments testing-float-flag 7";
  CHECK_EQ(0, FlagList::SetFlagsFromString(str, StrLength(str)));
  CHECK_EQ(42, FLAG_testing_int_flag);
  CHECK_EQ(2.5, FLAG_testing_float_flag);
  CHECK_EQ(2, FLAG_js_arguments.argc);
  CHECK_EQ(0, strcmp(FLAG_js_arguments[0], "testing-float-flag"));
  CHECK_EQ(0, strcmp(FLAG_js_arguments[1], "7"));
}


TEST(FlagsJSArguments3) {
  SetFlagsToDefault();
  const char* str = "--testing-int-flag 42 --js-arguments=testing-float-flag 7";
  CHECK_EQ(0, FlagList::SetFlagsFromString(str, StrLength(str)));
  CHECK_EQ(42, FLAG_testing_int_flag);
  CHECK_EQ(2.5, FLAG_testing_float_flag);
  CHECK_EQ(2, FLAG_js_arguments.argc);
  CHECK_EQ(0, strcmp(FLAG_js_arguments[0], "testing-float-flag"));
  CHECK_EQ(0, strcmp(FLAG_js_arguments[1], "7"));
}


TEST(FlagsJSArguments4) {
  SetFlagsToDefault();
  const char* str = "--testing-int-flag 42 --";
  CHECK_EQ(0, FlagList::SetFlagsFromString(str, StrLength(str)));
  CHECK_EQ(42, FLAG_testing_int_flag);
  CHECK_EQ(0, FLAG_js_arguments.argc);
}


TEST(FlagsRemoveIncomplete) {
  // Test that processed command line arguments are removed, even
  // if the list of arguments ends unexpectedly.
  SetFlagsToDefault();
  int argc = 3;
  const char* argv[] = { "", "--crankshaft", "--expose-debug-as" };
  CHECK_EQ(2, FlagList::SetFlagsFromCommandLine(&argc,
                                                const_cast<char **>(argv),
                                                true));
  CHECK_NE(NULL, argv[1]);
  CHECK_EQ(argc, 2);
}
