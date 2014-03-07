
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_function.hpp>
#endif

TT_TEST_BEGIN(is_function)

typedef void foo0_t();
typedef void foo1_t(int);
typedef void foo2_t(int&, double);
typedef void foo3_t(int&, bool, int, int);
typedef void foo4_t(int, bool, int*, int[], int, int, int, int, int);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<foo0_t>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<foo1_t>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<foo2_t>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<foo3_t>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<foo4_t>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<void>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<int>::value, false);
#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<int&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<int&&>::value, false);
#endif
#else
std::cout << 
"<note>is_function will fail with some types (references for example)"
"if the compiler doesn't support partial specialisation of class templates."
"These are *not* tested here</note>" << std::endl;
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<int*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<int[]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<test_abc1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<int (*)(int)>::value, false);

#ifdef BOOST_TT_TEST_MS_FUNC_SIGS
typedef void __stdcall sfoo0_t();
typedef void __stdcall sfoo1_t(int);
typedef void __stdcall sfoo2_t(int&, double);
typedef void __stdcall sfoo3_t(int&, bool, int, int);
typedef void __stdcall sfoo4_t(int, bool, int*, int[], int, int, int, int, int);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<sfoo0_t>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<sfoo1_t>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<sfoo2_t>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<sfoo3_t>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<sfoo4_t>::value, true);

#endif

TT_TEST_END








