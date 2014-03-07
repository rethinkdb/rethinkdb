
//  (C) Copyright John Maddock 2005. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/has_virtual_destructor.hpp>
#endif

#include <iostream>
#include <stdexcept>
#include <new>

class polymorphic_no_virtual_destructor
{
public:
   virtual void method() = 0;
};

TT_TEST_BEGIN(has_virtual_destructor)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<const int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<volatile int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<int*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<int* const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<int[2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<int&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<mf4>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<f1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<enum_UDT>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<empty_UDT>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<UDT*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<UDT[2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<UDT&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<UDT&&>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<void>::value, false);

BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<VB>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<VD>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<test_abc1>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<test_abc2>::value, true, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<polymorphic_no_virtual_destructor>::value, false);
#ifndef BOOST_NO_STD_LOCALE
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<std::iostream>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<std::basic_streambuf<char> >::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<std::basic_ios<char> >::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<std::basic_istream<char> >::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<std::basic_streambuf<char> >::value, true, false);
#endif
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<std::exception>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<std::bad_alloc>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<std::runtime_error>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<std::out_of_range>::value, true, false);
BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(::tt::has_virtual_destructor<std::range_error>::value, true, false);

TT_TEST_END








