// Copyright (c) 2008 Joseph Gauterin, Niels Dekker
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Tests swapping std::time_base::dateorder objects by means of boost::swap.
// std::time_base::dateorder is an enumerated type. It does not have an
// std::swap overload or template specialization.

#include <boost/utility/swap.hpp>
#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>

#include <locale>

int test_main(int, char*[])
{
  const std::time_base::dateorder initial_value1 = std::time_base::dmy;
  const std::time_base::dateorder initial_value2 = std::time_base::mdy;

  std::time_base::dateorder object1 = initial_value1;
  std::time_base::dateorder object2 = initial_value2;

  boost::swap(object1,object2);

  BOOST_CHECK_EQUAL(object1,initial_value2);
  BOOST_CHECK_EQUAL(object2,initial_value1);

  return 0;
}

