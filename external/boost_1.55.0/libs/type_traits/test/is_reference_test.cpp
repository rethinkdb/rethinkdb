
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_reference.hpp>
#endif

TT_TEST_BEGIN(is_reference)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<int&>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<const int&>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<volatile int &>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<const volatile int &>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<r_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<cr_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<UDT&>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<const UDT&>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<volatile UDT&>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<const volatile UDT&>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<int (&)(int)>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<int (&)[2]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<int [2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<const int [2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<volatile int [2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<const volatile int [2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<bool>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<void>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<test_abc1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<foo0_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<incomplete_type>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<int&&>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<UDT&&>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<const UDT&&>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<volatile UDT&&>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<const volatile UDT&&>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_reference<int (&&)[2]>::value, true);
#endif

TT_TEST_END








