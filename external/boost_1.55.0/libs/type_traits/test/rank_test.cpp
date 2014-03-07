
//  (C) Copyright John Maddock 2005. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/rank.hpp>
#endif

TT_TEST_BEGIN(rank)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::rank<int>::value, 0);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::rank<int[]>::value, 1);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::rank<int[][10]>::value, 2);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::rank<int[5][10]>::value, 2);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::rank<int[5][10][40]>::value, 3);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::rank<int (&)[5][10]>::value, 0);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::rank<int (*)[5][10]>::value, 0);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::rank<int (&&)[5][10]>::value, 0);
#endif

TT_TEST_END








