// Copyright (c) 2008 Joseph Gauterin, Niels Dekker
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Tests swapping an array of integers by means of boost::swap.

#include <boost/utility/swap.hpp>
#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>

#include <algorithm> //for std::copy and std::equal
#include <cstddef> //for std::size_t


int test_main(int, char*[])
{
  const std::size_t array_size = 3;
  const int initial_array1[array_size] = { 1, 2, 3 };
  const int initial_array2[array_size] = { 4, 5, 6 };
  
  int array1[array_size];
  int array2[array_size];

  std::copy(initial_array1, initial_array1 + array_size, array1);
  std::copy(initial_array2, initial_array2 + array_size, array2);
  
  boost::swap(array1, array2);

  BOOST_CHECK(std::equal(array1, array1 + array_size, initial_array2));
  BOOST_CHECK(std::equal(array2, array2 + array_size, initial_array1));

  return 0;
}
