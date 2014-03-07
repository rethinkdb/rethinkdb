
//  (C) Copyright John Maddock 2005. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_signed.hpp>
#endif

TT_TEST_BEGIN(is_signed)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_signed<int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_signed<long>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_signed<short>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_signed<signed char>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_signed<unsigned int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_signed<unsigned long>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_signed<unsigned short>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_signed<unsigned char>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_signed<UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_signed<int&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_signed<int&&>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_signed<int*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_signed<int[2]>::value, false);

#ifdef BOOST_HAS_INT128
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_signed<boost::int128_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_signed<boost::uint128_type>::value, false);
#endif
TT_TEST_END








