
//  (C) Copyright John Maddock 2005. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_floating_point.hpp>
#endif

TT_TEST_BEGIN(is_floating_point)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<float>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<float const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<float volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<float const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<double>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<double const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<double volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<double const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<long double>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<long double const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<long double volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<long double const volatile>::value, true);

//
// cases that should not be true:
//
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<void>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<test_abc1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<empty_UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<float*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<float&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<float&&>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<const float&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<float[2]>::value, false);

//
// tricky cases:
//
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<test_abc1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<foo0_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<foo1_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<foo2_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<foo3_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<foo4_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_floating_point<incomplete_type>::value, false);

TT_TEST_END








