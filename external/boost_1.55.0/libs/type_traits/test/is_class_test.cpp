
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_class.hpp>
#endif
#include <iostream>

TT_TEST_BEGIN(is_class)

   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<int>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<const int>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<volatile int>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<int*>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<int* const>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<int[2]>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<int const[2]>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<int&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<int&&>::value, false);
#endif
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<mf4>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<f1>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<enum_UDT>::value, false);

#if defined(BOOST_HAS_TYPE_TRAITS_INTRINSICS) && !defined(__sgi)
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<union_UDT>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<POD_union_UDT>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<empty_union_UDT>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<empty_POD_union_UDT>::value, false);
#else
   std::cout <<
   "\n<note>\n"
   "This compiler version does not provide support for is_class on\n"
   "union types. Such support is not currently required by the\n"
   "C++ Standard. It will be required to support the upcoming\n"
   "Standard Library Technical Report.\n"
   "</note>\n";
#endif

   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<UDT>::value, true);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<UDT const>::value, true);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<UDT volatile>::value, true);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<empty_UDT>::value, true);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<std::iostream>::value, true);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<test_abc1>::value, true);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<test_abc1 const>::value, true);

   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<UDT*>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<UDT[2]>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<UDT&>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<void>::value, false);

TT_TEST_END








