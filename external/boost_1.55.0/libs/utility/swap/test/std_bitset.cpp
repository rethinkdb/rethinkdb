// Copyright (c) 2008 - 2010 Joseph Gauterin, Niels Dekker
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Tests swapping std::bitset<T> objects by means of boost::swap.
// Unlike most other Standard C++ Library template classes,
// std::bitset<T> does not have its own std::swap overload.

#include <boost/utility/swap.hpp>
#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>

#include <bitset>

int test_main(int, char*[])
{
  typedef std::bitset<8> bitset_type;
  const bitset_type initial_value1 = 1;
  const bitset_type initial_value2 = 2;

  bitset_type object1 = initial_value1;
  bitset_type object2 = initial_value2;

  boost::swap(object1,object2);

  BOOST_CHECK_EQUAL(object1,initial_value2);
  BOOST_CHECK_EQUAL(object2,initial_value1);

  return 0;
}

