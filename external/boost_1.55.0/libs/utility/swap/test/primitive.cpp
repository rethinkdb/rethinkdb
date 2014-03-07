// Copyright (c) 2007 Joseph Gauterin
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/utility/swap.hpp>
#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>

int test_main(int, char*[])
{
  int object1 = 1;
  int object2 = 2;

  boost::swap(object1,object2);

  BOOST_CHECK_EQUAL(object1,2);
  BOOST_CHECK_EQUAL(object2,1);

  return 0;
}

