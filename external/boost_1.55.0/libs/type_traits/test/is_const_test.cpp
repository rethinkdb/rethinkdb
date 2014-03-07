
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_const.hpp>
#endif

TT_TEST_BEGIN(is_const)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<void>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<const void>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<test_abc1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<const test_abc1>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<const int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<const UDT>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<const volatile UDT>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<const int&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<const int&&>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<cr_type>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<incomplete_type>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<foo0_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<const int[2]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<int[2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<volatile int[2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<const volatile int[2]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<const volatile int[]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<volatile int[]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<const int[]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_const<int[]>::value, false);

TT_TEST_END








