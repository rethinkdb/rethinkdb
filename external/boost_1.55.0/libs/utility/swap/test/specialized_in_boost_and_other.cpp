// Copyright (c) 2008 Joseph Gauterin, Niels Dekker
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Tests whether instances of a class from a namespace other than boost are
// properly swapped, when both boost and the other namespace have a custom
// swap function for that class. Note that it shouldn't be necessary for a class
// in an other namespace to have a custom swap function in boost, because the
// boost::swap utility should find the swap function in the other namespace, by
// argument dependent lookup (ADL). Unfortunately ADL isn't fully implemented
// by some specific compiler versions, including Intel C++ 8.1, MSVC 7.1, and 
// Borland 5.9.3. Users of those compilers might consider adding a swap overload
// to the boost namespace.

#include <boost/utility/swap.hpp>
#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>

//Put test class in namespace other
namespace other
{
  #include "./swap_test_class.hpp"
}

//Provide swap function in namespace boost
namespace boost
{
  void swap(::other::swap_test_class& left, ::other::swap_test_class& right)
  {
    left.swap(right);
  }
}

//Provide swap function in namespace other
namespace other
{
  void swap(swap_test_class& left, swap_test_class& right)
  {
    left.swap(right);
  }
}

int test_main(int, char*[])
{
  const other::swap_test_class initial_value1(1);
  const other::swap_test_class initial_value2(2);

  other::swap_test_class object1 = initial_value1;
  other::swap_test_class object2 = initial_value2;
  
  other::swap_test_class::reset();
  boost::swap(object1,object2);

  BOOST_CHECK(object1 == initial_value2);
  BOOST_CHECK(object2 == initial_value1);
  
  BOOST_CHECK_EQUAL(other::swap_test_class::swap_count(),1);
  BOOST_CHECK_EQUAL(other::swap_test_class::copy_count(),0);

  return 0;
}

