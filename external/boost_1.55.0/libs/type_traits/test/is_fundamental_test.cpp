
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_fundamental.hpp>
#endif

TT_TEST_BEGIN(is_fundamental)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<void>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<void const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<void volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<void const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<bool>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<bool const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<bool volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<bool const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<signed char>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<signed char const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<signed char volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<signed char const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned char>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<char>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned char const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<char const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned char volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<char volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned char const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<char const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned short>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<short>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned short const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<short const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned short volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<short volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned short const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<short const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned int const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<int const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned int volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<int volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned int const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<int const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned long>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<long>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned long const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<long const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned long volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<long volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned long const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<long const volatile>::value, true);

#ifdef BOOST_HAS_LONG_LONG

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental< ::boost::ulong_long_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental< ::boost::long_long_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental< ::boost::ulong_long_type const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental< ::boost::long_long_type const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental< ::boost::ulong_long_type volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental< ::boost::long_long_type volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental< ::boost::ulong_long_type const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental< ::boost::long_long_type const volatile>::value, true);

#endif

#ifdef BOOST_HAS_MS_INT64

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned __int8>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<__int8>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned __int8 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<__int8 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned __int8 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<__int8 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned __int8 const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<__int8 const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned __int16>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<__int16>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned __int16 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<__int16 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned __int16 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<__int16 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned __int16 const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<__int16 const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned __int32>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<__int32>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned __int32 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<__int32 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned __int32 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<__int32 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned __int32 const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<__int32 const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned __int64>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<__int64>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned __int64 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<__int64 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned __int64 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<__int64 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<unsigned __int64 const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<__int64 const volatile>::value, true);

#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<float>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<float const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<float volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<float const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<double>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<double const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<double volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<double const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<long double>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<long double const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<long double volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<long double const volatile>::value, true);

//
// cases that should not be true:
//
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<test_abc1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<empty_UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<float*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<float&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<float&&>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<const float&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<float[2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<incomplete_type>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<foo0_t>::value, false);

#ifndef BOOST_NO_CXX11_CHAR16_T
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<char16_t>::value, true);
#endif
#ifndef BOOST_NO_CXX11_CHAR32_T
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_fundamental<char32_t>::value, true);
#endif

TT_TEST_END








