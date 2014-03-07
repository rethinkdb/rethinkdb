// Copyright (c) 2008 Joseph Gauterin, Niels Dekker
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Tests swapping std::string objects by means of boost::swap.
// std::string has its own std::swap overload.

#include <boost/utility/swap.hpp>
#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>

#include <string>

int test_main(int, char*[])
{
  const std::string initial_value1 = "one";
  const std::string initial_value2 = "two";

  std::string object1 = initial_value1;
  std::string object2 = initial_value2;

  boost::swap(object1,object2);

  BOOST_CHECK_EQUAL(object1,initial_value2);
  BOOST_CHECK_EQUAL(object2,initial_value1);

  return 0;
}

