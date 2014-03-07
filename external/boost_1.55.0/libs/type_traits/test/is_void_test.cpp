
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_void.hpp>
#endif

TT_TEST_BEGIN(is_void)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_void<void>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_void<void const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_void<void volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_void<void const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_void<void*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_void<int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_void<test_abc1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_void<foo0_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_void<foo1_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_void<foo2_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_void<foo3_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_void<foo4_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_void<incomplete_type>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_void<int&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_void<int&&>::value, false);
#endif

TT_TEST_END








