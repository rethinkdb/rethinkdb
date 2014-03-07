
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_pod.hpp>
#endif

TT_TEST_BEGIN(is_pod)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<bool>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<bool const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<bool volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<bool const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<signed char>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<signed char const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<signed char volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<signed char const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned char>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<char>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned char const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<char const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned char volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<char volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned char const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<char const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned short>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<short>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned short const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<short const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned short volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<short volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned short const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<short const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned int const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<int const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned int volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<int volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned int const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<int const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned long>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<long>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned long const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<long const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned long volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<long volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned long const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<long const volatile>::value, true);

#ifdef BOOST_HAS_LONG_LONG

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod< ::boost::ulong_long_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod< ::boost::long_long_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod< ::boost::ulong_long_type const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod< ::boost::long_long_type const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod< ::boost::ulong_long_type volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod< ::boost::long_long_type volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod< ::boost::ulong_long_type const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod< ::boost::long_long_type const volatile>::value, true);

#endif

#ifdef BOOST_HAS_MS_INT64

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned __int8>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<__int8>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned __int8 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<__int8 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned __int8 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<__int8 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned __int8 const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<__int8 const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned __int16>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<__int16>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned __int16 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<__int16 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned __int16 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<__int16 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned __int16 const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<__int16 const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned __int32>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<__int32>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned __int32 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<__int32 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned __int32 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<__int32 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned __int32 const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<__int32 const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned __int64>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<__int64>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned __int64 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<__int64 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned __int64 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<__int64 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<unsigned __int64 const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<__int64 const volatile>::value, true);

#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<float>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<float const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<float volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<float const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<double>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<double const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<double volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<double const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<long double>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<long double const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<long double volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<long double const volatile>::value, true);


BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<void*>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<int*const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<f1>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<f2>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<f3>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<mf1>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<mf2>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<mf3>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<mp>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<cmf>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<enum_UDT>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<int&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<int&&>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<const int&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<int[2]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<int[3][2]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<int[2][4][5][6][3]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<empty_UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<void>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<test_abc1>::value, false);

// cases we would like to succeed but can't implement in the language:
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_pod<empty_POD_UDT>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_pod<POD_UDT>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_pod<POD_union_UDT>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_pod<empty_POD_union_UDT>::value, true, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<trivial_except_copy>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<trivial_except_destroy>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<trivial_except_assign>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<trivial_except_construct>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<wrap<trivial_except_copy> >::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<wrap<trivial_except_assign> >::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<wrap<trivial_except_destroy> >::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<wrap<trivial_except_construct> >::value, false);

#ifndef BOOST_NO_CXX11_CHAR16_T
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<char16_t>::value, true);
#endif
#ifndef BOOST_NO_CXX11_CHAR32_T
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<char32_t>::value, true);
#endif

TT_TEST_END








