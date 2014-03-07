
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_lvalue_reference.hpp>
#endif

TT_TEST_BEGIN(is_lvalue_reference)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<int&>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<const int&>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<volatile int &>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<const volatile int &>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<r_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<cr_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<UDT&>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<const UDT&>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<volatile UDT&>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<const volatile UDT&>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<int (&)(int)>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<int (&)[2]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<int [2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<const int [2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<volatile int [2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<const volatile int [2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<bool>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<void>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<test_abc1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<foo0_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<incomplete_type>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<int&&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<UDT&&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<const UDT&&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<volatile UDT&&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<const volatile UDT&&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<int (&&)(int)>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_lvalue_reference<int (&&)[2]>::value, false);
#endif

TT_TEST_END








