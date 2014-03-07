
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_union.hpp>
#endif
#include <iostream>

TT_TEST_BEGIN(is_union)

   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<int>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<const int>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<volatile int>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<int*>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<int* const>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<int[2]>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<int&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<int&&>::value, false);
#endif
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<mf4>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<f1>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<enum_UDT>::value, false);

#if defined(BOOST_HAS_TYPE_TRAITS_INTRINSICS) && !defined(__sgi)
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<union_UDT>::value, true);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<POD_union_UDT>::value, true);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<empty_union_UDT>::value, true);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<empty_POD_union_UDT>::value, true);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<union_UDT const>::value, true);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<POD_union_UDT volatile>::value, true);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<empty_union_UDT const volatile>::value, true);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<empty_POD_union_UDT const>::value, true);
#else
   std::cout <<
   "\n<note>\n"
   "This compiler version does not provide support for is_union on\n"
   "union types. Such support is not currently required by the\n"
   "C++ Standard. It will be required to support the upcoming\n"
   "Standard Library Technical Report.\n"
   "</note>\n";
#endif

   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<UDT>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<UDT const>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<UDT volatile>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<empty_UDT>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<std::iostream>::value, false);

   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<UDT*>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<UDT[2]>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<UDT&>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<void>::value, false);

   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<test_abc1>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<foo0_t>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<foo1_t>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<foo2_t>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<foo3_t>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<foo4_t>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<incomplete_type>::value, false);

TT_TEST_END









