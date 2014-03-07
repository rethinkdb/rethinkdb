
//  (C) Copyright John Maddock 2005. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/extent.hpp>
#endif

TT_TEST_BEGIN(extent)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::extent<int>::value, 0);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::extent<int[]>::value, 0);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::extent<int[][10]>::value, 0);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::extent<int[5][10]>::value, 5);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::extent<int[5][10][40]>::value, 5);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::extent<int (&)[5][10]>::value, 0);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::extent<int (*)[5][10]>::value, 0);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::extent<int,1>::value), 0);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::extent<int[],1>::value), 0);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::extent<int[][10],1>::value), 10);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::extent<int[5][10],1>::value), 10);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::extent<int[5][10][40],1>::value), 10);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::extent<int (&)[5][10],1>::value), 0);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::extent<int (*)[5][10],1>::value), 0);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::extent<int[5][10],2>::value), 0);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::extent<int[5][10][40],2>::value), 40);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::extent<int[5][10][40],3>::value), 0);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::extent<int*>::value, 0);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::extent<int&>::value, 0);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::extent<int&&>::value, 0);
#endif

TT_TEST_END








