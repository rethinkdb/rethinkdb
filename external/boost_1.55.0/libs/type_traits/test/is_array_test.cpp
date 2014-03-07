
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_array.hpp>
#endif

struct convertible_to_pointer
{
    operator char*() const;
};

TT_TEST_BEGIN(is_array)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<int*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<const int*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<const volatile int*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<int*const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<const int*volatile>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<const volatile int*const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<int[2]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<const int[2]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<const volatile int[2]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<int[2][3]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<UDT[2]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<int(&)[2]>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<int(&&)[2]>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<f1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<void>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<test_abc1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<convertible_to_pointer>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<test_abc1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<foo0_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<incomplete_type>::value, false);


TT_TEST_END








