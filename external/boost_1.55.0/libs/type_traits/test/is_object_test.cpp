
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_object.hpp>
#endif

TT_TEST_BEGIN(is_object)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_object<int>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_object<UDT>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_object<int&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_object<int&&>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_object<void>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_object<foo4_t>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_object<test_abc1>::value, true);
// this one is only partly correct:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_object<incomplete_type>::value, true);


TT_TEST_END








