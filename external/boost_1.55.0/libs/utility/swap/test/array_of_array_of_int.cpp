// Copyright (c) 2008 Joseph Gauterin, Niels Dekker
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Tests swapping an array of arrays of integers by means of boost::swap.

#include <boost/utility/swap.hpp>
#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>

#include <algorithm> //for std::copy and std::equal
#include <cstddef> //for std::size_t

int test_main(int, char*[])
{
  const std::size_t first_dimension = 3;
  const std::size_t second_dimension = 4;
  const std::size_t number_of_elements = first_dimension * second_dimension;

  int array1[first_dimension][second_dimension];
  int array2[first_dimension][second_dimension];

  int* const ptr1 = array1[0];
  int* const ptr2 = array2[0];

  for (std::size_t i = 0; i < number_of_elements; ++i)
  {
    ptr1[i] = static_cast<int>(i);
    ptr2[i] = static_cast<int>(i + number_of_elements);
  }

  boost::swap(array1, array2);

  for (std::size_t i = 0; i < number_of_elements; ++i)
  {
    BOOST_CHECK_EQUAL(ptr1[i], static_cast<int>(i + number_of_elements) );
    BOOST_CHECK_EQUAL(ptr2[i], static_cast<int>(i) );
  }
  return 0;
}
