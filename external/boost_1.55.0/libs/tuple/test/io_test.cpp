// Copyright (C) 1999, 2000 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

// -- io_test.cpp -----------------------------------------------
//
// Testing the I/O facilities of tuples

#define BOOST_INCLUDE_MAIN  // for testing, include rather than link
#include "boost/test/test_tools.hpp"    // see "Header Implementation Option"

#include "boost/tuple/tuple_io.hpp"
#include "boost/tuple/tuple_comparison.hpp"

#include <fstream>
#include <iterator>
#include <algorithm>
#include <string>
#include <iomanip>

#if defined BOOST_NO_STRINGSTREAM
#include <strstream>
#else
#include <sstream>
#endif

using namespace boost;

#if defined BOOST_NO_STRINGSTREAM
typedef std::ostrstream useThisOStringStream;
typedef std::istrstream useThisIStringStream;
#else
typedef std::ostringstream useThisOStringStream;
typedef std::istringstream useThisIStringStream;
#endif

int test_main(int argc, char * argv[] ) {
   (void)argc;
   (void)argv;
   using boost::tuples::set_close;
   using boost::tuples::set_open;
   using boost::tuples::set_delimiter;
   
  useThisOStringStream os1;

  // Set format [a, b, c] for os1
  os1 << set_open('[');
  os1 << set_close(']');
  os1 << set_delimiter(',');
  os1 << make_tuple(1, 2, 3);
  BOOST_CHECK (os1.str() == std::string("[1,2,3]") );

  {
  useThisOStringStream os2;
  // Set format (a:b:c) for os2; 
  os2 << set_open('(');
  os2 << set_close(')');
  os2 << set_delimiter(':');
#if !defined (BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
  os2 << make_tuple("TUPU", "HUPU", "LUPU", 4.5);
  BOOST_CHECK (os2.str() == std::string("(TUPU:HUPU:LUPU:4.5)") );
#endif
  }

  // The format is still [a, b, c] for os1
  os1 << make_tuple(1, 2, 3);
  BOOST_CHECK (os1.str() == std::string("[1,2,3][1,2,3]") );

  // check empty tuple.
  useThisOStringStream os3;
  os3 << make_tuple();
  BOOST_CHECK (os3.str() == std::string("()") );
  os3 << set_open('[');
  os3 << set_close(']');
  os3 << make_tuple();
  BOOST_CHECK (os3.str() == std::string("()[]") );
  
  // check width
  useThisOStringStream os4;
  os4 << std::setw(10) << make_tuple(1, 2, 3);
  BOOST_CHECK (os4.str() == std::string("   (1 2 3)") );

  std::ofstream tmp("temp.tmp");

#if !defined (BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
  tmp << make_tuple("One", "Two", 3);
#endif   
  tmp << set_delimiter(':');
  tmp << make_tuple(1000, 2000, 3000) << std::endl;

  tmp.close();
  
  // When teading tuples from a stream, manipulators must be set correctly:
  std::ifstream tmp3("temp.tmp");
  tuple<std::string, std::string, int> j;

#if !defined (BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
  tmp3 >> j; 
  BOOST_CHECK (tmp3.good() ); 
#endif
   
  tmp3 >> set_delimiter(':');
  tuple<int, int, int> i;
  tmp3 >> i; 
  BOOST_CHECK (tmp3.good() ); 
   
  tmp3.close(); 


  // reading tuple<int, int, int> in format (a b c); 
  useThisIStringStream is1("(100 200 300)"); 
   
  tuple<int, int, int> ti1; 
  BOOST_CHECK(bool(is1 >> ti1));
  BOOST_CHECK(ti1 == make_tuple(100, 200, 300));

  useThisIStringStream is2("()");
  tuple<> ti2;
  BOOST_CHECK(bool(is2 >> ti2));
  useThisIStringStream is3("[]");
  is3 >> set_open('[');
  is3 >> set_close(']');
  BOOST_CHECK(bool(is3 >> ti2));

  // Make sure that whitespace between elements
  // is skipped.
  useThisIStringStream is4("(100 200 300)"); 
   
  BOOST_CHECK(bool(is4 >> std::noskipws >> ti1));
  BOOST_CHECK(ti1 == make_tuple(100, 200, 300));

  // Note that strings are problematic:
  // writing a tuple on a stream and reading it back doesn't work in
  // general. If this is wanted, some kind of a parseable string class
  // should be used.
  
  return 0;
}

