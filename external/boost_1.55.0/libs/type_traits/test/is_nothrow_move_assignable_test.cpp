
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
#  include <boost/type_traits/is_nothrow_move_assignable.hpp>
#endif

TT_TEST_BEGIN(is_nothrow_move_assignable)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<bool>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<bool const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<bool volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<bool const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<signed char>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<signed char const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<signed char volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<signed char const volatile>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned char>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<char>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned char const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<char const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned char volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<char volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned char const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<char const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned short>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<short>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned short const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<short const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned short volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<short volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned short const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<short const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<int>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned int const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<int const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned int volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<int volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned int const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<int const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned long>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<long>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned long const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<long const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned long volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<long volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned long const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<long const volatile>::value, false);
#endif

#ifdef BOOST_HAS_LONG_LONG

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable< ::boost::ulong_long_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable< ::boost::long_long_type>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable< ::boost::ulong_long_type const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable< ::boost::long_long_type const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable< ::boost::ulong_long_type volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable< ::boost::long_long_type volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable< ::boost::ulong_long_type const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable< ::boost::long_long_type const volatile>::value, false);
#endif
#endif

#ifdef BOOST_HAS_MS_INT64

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned __int8>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<__int8>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned __int8 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<__int8 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned __int8 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<__int8 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned __int8 const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<__int8 const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned __int16>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<__int16>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned __int16 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<__int16 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned __int16 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<__int16 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned __int16 const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<__int16 const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned __int32>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<__int32>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned __int32 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<__int32 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned __int32 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<__int32 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned __int32 const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<__int32 const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned __int64>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<__int64>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned __int64 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<__int64 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned __int64 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<__int64 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<unsigned __int64 const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<__int64 const volatile>::value, false);
#endif
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<float>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<float const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<float volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<float const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<double>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<double const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<double volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<double const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<long double>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<long double const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<long double volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<long double const volatile>::value, false);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<void*>::value, true);
#ifndef TEST_STD
// unspecified behaviour:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<int*const>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<f1>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<f2>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<f3>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<mf1>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<mf2>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<mf3>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<mp>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<cmf>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<enum_UDT>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<int&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<int&&>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<const int&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<int[2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<int[3][2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<int[2][4][5][6][3]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<empty_UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<void>::value, false);
// cases we would like to succeed but can't implement in the language:
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<empty_POD_UDT>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<POD_UDT>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<POD_union_UDT>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<empty_POD_union_UDT>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<nothrow_assign_UDT>::value, true, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<nothrow_copy_UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<nothrow_construct_UDT>::value, false);

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_NOEXCEPT)
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<nothrow_move_UDT>::value, true);
#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_nothrow_move_assignable<test_abc1>::value, false);

TT_TEST_END



