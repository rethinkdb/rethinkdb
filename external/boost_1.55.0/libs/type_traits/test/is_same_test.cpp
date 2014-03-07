
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_same.hpp>
#endif

TT_TEST_BEGIN(is_same)

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<int, int>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<int&, int&>::value), true);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<int&&, int&&>::value), true);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<int, const int>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<int, int&>::value), false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<int, int&&>::value), false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<const int, int&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<int, const int&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<int*, const int*>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<int*, int*const>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<int, int[2]>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<int*, int[2]>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same<int[4], int[2]>::value), false);

TT_TEST_END








