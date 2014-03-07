
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/has_trivial_constructor.hpp>
#endif

TT_TEST_BEGIN(has_trivial_constructor)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<bool>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<bool const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<bool volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<bool const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<signed char>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<signed char const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<signed char volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<signed char const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned char>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<char>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned char const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<char const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned char volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<char volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned char const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<char const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned short>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<short>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned short const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<short const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned short volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<short volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned short const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<short const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned int const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<int const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned int volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<int volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned int const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<int const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned long>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<long>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned long const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<long const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned long volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<long volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned long const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<long const volatile>::value, true);

#ifdef BOOST_HAS_LONG_LONG

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor< ::boost::ulong_long_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor< ::boost::long_long_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor< ::boost::ulong_long_type const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor< ::boost::long_long_type const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor< ::boost::ulong_long_type volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor< ::boost::long_long_type volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor< ::boost::ulong_long_type const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor< ::boost::long_long_type const volatile>::value, true);

#endif

#ifdef BOOST_HAS_MS_INT64

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned __int8>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<__int8>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned __int8 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<__int8 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned __int8 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<__int8 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned __int8 const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<__int8 const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned __int16>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<__int16>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned __int16 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<__int16 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned __int16 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<__int16 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned __int16 const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<__int16 const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned __int32>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<__int32>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned __int32 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<__int32 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned __int32 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<__int32 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned __int32 const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<__int32 const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned __int64>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<__int64>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned __int64 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<__int64 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned __int64 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<__int64 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<unsigned __int64 const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<__int64 const volatile>::value, true);

#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<float>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<float const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<float volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<float const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<double>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<double const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<double volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<double const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<long double>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<long double const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<long double volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<long double const volatile>::value, true);


BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<void*>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<int*const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<f1>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<f2>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<f3>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<mf1>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<mf2>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<mf3>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<mp>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<cmf>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<enum_UDT>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<int&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<int&&>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<const int&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<int[2]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<int[3][2]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<int[2][4][5][6][3]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<empty_UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<void>::value, true);
// cases we would like to succeed but can't implement in the language:
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<empty_POD_UDT>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<POD_UDT>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<POD_union_UDT>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<empty_POD_union_UDT>::value, true, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<trivial_except_construct>::value, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<trivial_except_destroy>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<trivial_except_assign>::value, true, false);
//BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<trivial_except_copy>::value, true, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<wrap<trivial_except_construct> >::value, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<wrap<trivial_except_destroy> >::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<wrap<trivial_except_assign> >::value, true, false);
//BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<wrap<trivial_except_copy> >::value, true, false);


BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_trivial_constructor<test_abc1>::value, false);

TT_TEST_END








