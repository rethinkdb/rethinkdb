
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_float.hpp>
#endif

TT_TEST_BEGIN(is_float)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<float>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<float const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<float volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<float const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<double>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<double const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<double volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<double const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<long double>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<long double const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<long double volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<long double const volatile>::value, true);

//
// cases that should not be true:
//
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<void>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<test_abc1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<empty_UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<float*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<float&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<float&&>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<const float&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<float[2]>::value, false);

//
// tricky cases:
//
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<test_abc1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<foo0_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<foo1_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<foo2_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<foo3_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<foo4_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_float<incomplete_type>::value, false);

TT_TEST_END








