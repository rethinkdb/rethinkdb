
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_empty.hpp>
#endif

struct non_default_constructable_UDT
{
   non_default_constructable_UDT(const non_default_constructable_UDT&){}
private:
   non_default_constructable_UDT(){}
};

TT_TEST_BEGIN(is_empty)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_empty<int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_empty<int*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_empty<int&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_empty<int&&>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_empty<void>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_empty<int[2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_empty<f1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_empty<mf1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_empty<UDT>::value, false);

BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_empty<empty_UDT>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_empty<empty_POD_UDT>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_empty<non_default_constructable_UDT>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::is_empty<boost::noncopyable>::value, true, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_empty<enum_UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_empty<non_empty>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_empty<const non_empty&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_empty<foo4_t>::value, false);

TT_TEST_END








