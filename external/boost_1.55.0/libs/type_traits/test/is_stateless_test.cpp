
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_stateless.hpp>
#endif

TT_TEST_BEGIN(is_stateless)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<bool>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<bool const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<bool volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<bool const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<signed char>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<signed char const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<signed char volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<signed char const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned char>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<char>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned char const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<char const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned char volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<char volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned char const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<char const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned short>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<short>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned short const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<short const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned short volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<short volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned short const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<short const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned int const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<int const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned int volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<int volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned int const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<int const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned long>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<long>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned long const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<long const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned long volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<long volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned long const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<long const volatile>::value, false);

#ifdef BOOST_HAS_LONG_LONG

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless< ::boost::ulong_long_type>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless< ::boost::long_long_type>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless< ::boost::ulong_long_type const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless< ::boost::long_long_type const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless< ::boost::ulong_long_type volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless< ::boost::long_long_type volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless< ::boost::ulong_long_type const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless< ::boost::long_long_type const volatile>::value, false);

#endif

#ifdef BOOST_HAS_MS_INT64

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned __int8>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<__int8>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned __int8 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<__int8 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned __int8 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<__int8 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned __int8 const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<__int8 const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned __int16>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<__int16>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned __int16 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<__int16 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned __int16 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<__int16 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned __int16 const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<__int16 const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned __int32>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<__int32>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned __int32 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<__int32 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned __int32 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<__int32 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned __int32 const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<__int32 const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned __int64>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<__int64>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned __int64 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<__int64 const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned __int64 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<__int64 volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<unsigned __int64 const volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<__int64 const volatile>::value, false);

#endif

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<float>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<float const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<float volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<float const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<double>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<double const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<double volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<double const volatile>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<long double>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<long double const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<long double volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<long double const volatile>::value, false);


BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<void*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<int*const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<f1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<f2>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<f3>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<mf1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<mf2>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<mf3>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<mp>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<cmf>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<enum_UDT>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<int&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<int&&>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<const int&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<int[2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<int[3][2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<int[2][4][5][6][3]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<empty_UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<void>::value, false);

// cases we would like to succeed but can't implement in the language:
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_stateless<empty_POD_UDT>::value, true, false);


TT_TEST_END








