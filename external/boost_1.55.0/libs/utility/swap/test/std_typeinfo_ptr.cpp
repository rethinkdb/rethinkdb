// Copyright (c) 2008 Joseph Gauterin, Niels Dekker
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Tests swapping std::type_info pointers by means of boost::swap.
// There is no std::swap overload or template specialization
// for std::type_info pointers.

#include <boost/utility/swap.hpp>
#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>

#include <typeinfo>

int test_main(int, char*[])
{
  const std::type_info * const initial_value1 = 0;
  const std::type_info * const initial_value2 = &typeid(double);
  
  const std::type_info * ptr1 = initial_value1;
  const std::type_info * ptr2 = initial_value2;

  boost::swap(ptr1,ptr2);

  BOOST_CHECK_EQUAL(ptr1,initial_value2);
  BOOST_CHECK_EQUAL(ptr2,initial_value1);

  return 0;
}

