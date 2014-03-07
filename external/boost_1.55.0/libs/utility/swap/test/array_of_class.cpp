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
  const std::size_t array_size = 2;
  const swap_test_class initial_array1[array_size] = { swap_test_class(1), swap_test_class(2) };
  const swap_test_class initial_array2[array_size] = { swap_test_class(3), swap_test_class(4) };
  
  swap_test_class array1[array_size];
  swap_test_class array2[array_size];

  std::copy(initial_array1, initial_array1 + array_size, array1);
  std::copy(initial_array2, initial_array2 + array_size, array2);
  
  swap_test_class::reset();
  boost::swap(array1, array2);

  BOOST_CHECK(std::equal(array1, array1 + array_size, initial_array2));
  BOOST_CHECK(std::equal(array2, array2 + array_size, initial_array1));

  BOOST_CHECK_EQUAL(swap_test_class::swap_count(), array_size);
  BOOST_CHECK_EQUAL(swap_test_class::copy_count(), 0);

  return 0;
}
