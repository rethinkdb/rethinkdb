
//  (C) Copyright John Maddock 2000. 
//  (C) Copyright Antony Polukhin 2013.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_nothrow_move_constructible.hpp>
#endif

TT_TEST_BEGIN(is_nothrow_move_constructible)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<bool>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<bool const>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<bool volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<bool const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<signed char>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<signed char const>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<signed char volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<signed char const volatile>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned char>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<char>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned char const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<char const>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned char volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<char volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned char const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<char const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned short>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<short>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned short const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<short const>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned short volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<short volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned short const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<short const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned int const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<int const>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned int volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<int volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned int const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<int const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned long>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<long>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned long const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<long const>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned long volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<long volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned long const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<long const volatile>::value, false);
#endif

#ifdef BOOST_HAS_LONG_LONG

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible< ::boost::ulong_long_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible< ::boost::long_long_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible< ::boost::ulong_long_type const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible< ::boost::long_long_type const>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible< ::boost::ulong_long_type volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible< ::boost::long_long_type volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible< ::boost::ulong_long_type const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible< ::boost::long_long_type const volatile>::value, false);
#endif
#endif

#ifdef BOOST_HAS_MS_INT64

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned __int8>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<__int8>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned __int8 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<__int8 const>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned __int8 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<__int8 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned __int8 const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<__int8 const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned __int16>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<__int16>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned __int16 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<__int16 const>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned __int16 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<__int16 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned __int16 const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<__int16 const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned __int32>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<__int32>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned __int32 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<__int32 const>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned __int32 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<__int32 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned __int32 const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<__int32 const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned __int64>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<__int64>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned __int64 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<__int64 const>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned __int64 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<__int64 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<unsigned __int64 const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<__int64 const volatile>::value, false);
#endif
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<float>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<float const>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<float volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<float const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<double>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<double const>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<double volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<double const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<long double>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<long double const>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<long double volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<long double const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<void*>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<int*const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<f1>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<f2>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<f3>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<mf1>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<mf2>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<mf3>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<mp>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<cmf>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<enum_UDT>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<int&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<int&&>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<const int&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<int[2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<int[3][2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<int[2][4][5][6][3]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<void>::value, false);
// cases we would like to succeed but can't implement in the language:
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<empty_POD_UDT>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<POD_UDT>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<POD_union_UDT>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<empty_POD_union_UDT>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<nothrow_copy_UDT>::value, true, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<nothrow_assign_UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<nothrow_construct_UDT>::value, false);

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_NOEXCEPT)
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<nothrow_move_UDT>::value, true);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_constructible<test_abc1>::value, false);

TT_TEST_END



