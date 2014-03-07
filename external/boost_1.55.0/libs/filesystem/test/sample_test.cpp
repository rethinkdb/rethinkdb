//  filesystem sample_test.cpp  ----------------------------------------------//

//  Copyright Beman Dawes 2012

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  --------------------------------------------------------------------------//
//
//  This program provides a template for bug reporting test cases.
//
//  --------------------------------------------------------------------------//

#include <boost/config/warning_disable.hpp>
#include <boost/filesystem.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <iostream>
#include <string>
#include <cstring>
#ifndef BOOST_LIGHTWEIGHT_MAIN
#  include <boost/test/prg_exec_monitor.hpp>
#else
#  include <boost/detail/lightweight_main.hpp>
#endif

namespace fs = boost::filesystem;
using fs::path;
using std::cout;
using std::endl;
using std::string;
using std::wstring;

namespace
{
  bool cleanup = true;
}

//  cpp_main  ----------------------------------------------------------------//

int cpp_main(int argc, char* argv[])
{
  if (argc > 1 && std::strcmp(argv[1], "--no-cleanup") == 0)
    cleanup = false;

  //  Test cases go after this block of comments
  //    Use test case macros from boost/detail/lightweight_test.hpp:
  //
  //    BOOST_TEST(predicate);  // test passes if predicate evaluates to true
  //    BOOST_TEST_EQ(x, y);    // test passes if x == y
  //    BOOST_TEST_NE(x, y);    // test passes if x != y
  //    BOOST_ERROR(msg);       // test fails, outputs msg 
  //    Examples:
  //      BOOST_TEST(path("f00").size() == 3);   // test passes
  //      BOOST_TEST_EQ(path("f00").size(), 3);  // test passes
  //      BOOST_MSG("Oops!");  // test fails, outputs "Oops!"

  if (cleanup)
  {
    // Remove any test files or directories here
  }

  return ::boost::report_errors();
}
