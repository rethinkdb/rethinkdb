// Copyright (c) 2008 Joseph Gauterin, Niels Dekker
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Tests swapping std::vector objects by means of boost::swap,
// having ::swap_test_class as vector element type. 

#include <boost/utility/swap.hpp>
#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>

#include <vector>

//Put test class in the global namespace
#include "./swap_test_class.hpp"

//Provide swap function in the global namespace 
void swap(swap_test_class& left, swap_test_class& right)
{
  left.swap(right);
}

int test_main(int, char*[])
{
  typedef std::vector<swap_test_class> vector_type;

  const vector_type::size_type initial_size1 = 1;
  const vector_type::size_type initial_size2 = 2;

  const vector_type initial_value1(initial_size1, swap_test_class(1));
  const vector_type initial_value2(initial_size2, swap_test_class(2));
  
  vector_type object1 = initial_value1;
  vector_type object2 = initial_value2;

  swap_test_class::reset();
  
  boost::swap(object1,object2);

  BOOST_CHECK_EQUAL(object1.size(),initial_size2);
  BOOST_CHECK_EQUAL(object2.size(),initial_size1);

  BOOST_CHECK(object1 == initial_value2);
  BOOST_CHECK(object2 == initial_value1);

  BOOST_CHECK_EQUAL(swap_test_class::swap_count(),0);
  BOOST_CHECK_EQUAL(swap_test_class::copy_count(),0);

  return 0;
}

