// Copyright 2006, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
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
//
// Author: phanna@google.com (Patrick Hanna)

// Unit test for Google Test's --gtest_list_tests flag.
//
// A user can ask Google Test to list all tests that will run
// so that when using a filter, a user will know what
// tests to look for. The tests will not be run after listing.
//
// This program will be invoked from a Python unit test.
// Don't run it directly.

#include "gtest/gtest.h"

namespace {

// Several different test cases and tests that will be listed.
TEST(Foo, Bar1) {
}

TEST(Foo, Bar2) {
}

TEST(Foo, DISABLED_Bar3) {
}

TEST(Abc, Xyz) {
}

TEST(Abc, Def) {
}

TEST(FooBar, Baz) {
}

class FooTest : public testing::Test {
};

TEST_F(FooTest, Test1) {
}

TEST_F(FooTest, DISABLED_Test2) {
}

TEST_F(FooTest, Test3) {
}

TEST(FooDeathTest, Test1) {
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
