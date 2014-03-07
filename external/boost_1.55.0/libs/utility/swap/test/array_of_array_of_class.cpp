// Copyright (c) 2008 Joseph Gauterin, Niels Dekker
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Tests swapping an array of arrays of swap_test_class objects by means of boost::swap.

#include <boost/utility/swap.hpp>
#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>

//Put test class in the global namespace
#include "./swap_test_class.hpp"

#include <algorithm> //for std::copy and std::equal
#include <cstddef> //for std::size_t

//Provide swap function in both the namespace of swap_test_class
//(which is the global namespace), and the std namespace.
//It's common to provide a swap function for a class in both
//namespaces. Scott Meyers recommends doing so: Effective C++,
//Third Edition, item 25, "Consider support for a non-throwing swap".
void swap(swap_test_class& left, swap_test_class& right)
{
  left.swap(right);
}

namespace std
{
  template <>
  void swap(swap_test_class& left, swap_test_class& right)
  {
    left.swap(right);
  }
}


int test_main(int, char*[])
{
  const std::size_t first_dimension = 3;
  const std::size_t second_dimension = 4;
  const std::size_t number_of_elements = first_dimension * second_dimension;

  swap_test_class array1[first_dimension][second_dimension];
  swap_test_class array2[first_dimension][second_dimension];

  swap_test_class* const ptr1 = array1[0];
  swap_test_class* const ptr2 = array2[0];

  for (std::size_t i = 0; i < number_of_elements; ++i)
  {
    ptr1[i].set_data( static_cast<int>(i) );
    ptr2[i].set_data( static_cast<int>(i + number_of_elements) );
  }

  boost::swap(array1, array2);

  for (std::size_t i = 0; i < number_of_elements; ++i)
  {
    BOOST_CHECK_EQUAL(ptr1[i].get_data(), static_cast<int>(i + number_of_elements) );
    BOOST_CHECK_EQUAL(ptr2[i].get_data(), static_cast<int>(i) );
  }

  BOOST_CHECK_EQUAL(swap_test_class::swap_count(), number_of_elements);
  BOOST_CHECK_EQUAL(swap_test_class::copy_count(), 0);

  return 0;
}
