
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_integral.hpp>
#endif

TT_TEST_BEGIN(is_integral)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<bool>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<bool const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<bool volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<bool const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<signed char>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<signed char const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<signed char volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<signed char const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned char>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<char>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned char const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<char const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned char volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<char volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned char const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<char const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned short>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<short>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned short const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<short const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned short volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<short volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned short const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<short const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned int const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<int const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned int volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<int volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned int const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<int const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned long>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<long>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned long const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<long const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned long volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<long volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned long const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<long const volatile>::value, true);

#ifdef BOOST_HAS_LONG_LONG

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral< ::boost::ulong_long_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral< ::boost::long_long_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral< ::boost::ulong_long_type const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral< ::boost::long_long_type const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral< ::boost::ulong_long_type volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral< ::boost::long_long_type volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral< ::boost::ulong_long_type const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral< ::boost::long_long_type const volatile>::value, true);

#endif

#ifdef BOOST_HAS_MS_INT64

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned __int8>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<__int8>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned __int8 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<__int8 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned __int8 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<__int8 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned __int8 const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<__int8 const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned __int16>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<__int16>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned __int16 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<__int16 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned __int16 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<__int16 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned __int16 const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<__int16 const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned __int32>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<__int32>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned __int32 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<__int32 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned __int32 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<__int32 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned __int32 const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<__int32 const volatile>::value, true);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned __int64>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<__int64>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned __int64 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<__int64 const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned __int64 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<__int64 volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<unsigned __int64 const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<__int64 const volatile>::value, true);

#endif

#ifdef BOOST_HAS_INT128

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<boost::uint128_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<boost::int128_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<boost::uint128_type const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<boost::int128_type const>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<boost::uint128_type volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<boost::int128_type volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<boost::uint128_type const volatile>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<boost::int128_type const volatile>::value, true);

#endif
//
// cases that should not be true:
//
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<void>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<float>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<test_abc1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<empty_UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<int*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<int&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<int&&>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<const int&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<int[2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<test_abc1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<foo0_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<foo1_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<foo2_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<foo3_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<foo4_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<incomplete_type>::value, false);

#ifndef BOOST_NO_CXX11_CHAR16_T
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<char16_t>::value, true);
#endif
#ifndef BOOST_NO_CXX11_CHAR32_T
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_integral<char32_t>::value, true);
#endif

TT_TEST_END

