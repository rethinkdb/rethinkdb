
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_polymorphic.hpp>
#endif

#include <exception>
#include <new>
#include <stdexcept>

#if defined(BOOST_WINDOWS) && !defined(BOOST_DISABLE_WIN32)
#include <windows.h> // more things to test
#endif

#if defined(BOOST_MSVC) && (_MSC_VER >= 1700)
#pragma warning(disable:4250)
#endif

// this test was added to check for bug reported on 21 May 2003:
struct poly_bug { virtual int foo() = 0; };

TT_TEST_BEGIN(is_polymorphic)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<const int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<volatile int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<int*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<int* const>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<int[2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<int&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<int&&>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<mf4>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<f1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<enum_UDT>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<UDT>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<empty_UDT>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<UDT*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<UDT[2]>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<UDT&>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<void>::value, false);

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<VB>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<VD>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<test_abc1>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<test_abc2>::value, true);
#ifndef BOOST_NO_STD_LOCALE
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<std::iostream>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<std::basic_streambuf<char> >::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<std::basic_ios<char> >::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<std::basic_istream<char> >::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<std::basic_streambuf<char> >::value, true);
// some libraries don't make this one a polymorphic class, since this is a library
// rather than a type traits issue we won't test this for now...
//BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<std::ios_base>::value, true);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<std::exception>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<std::bad_alloc>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<std::runtime_error>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<std::out_of_range>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<std::range_error>::value, true);

#if defined(BOOST_WINDOWS) && !defined(BOOST_DISABLE_WIN32)
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<IUnknown>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<ITypeInfo>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<ITypeComp>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<IDispatch>::value, true);
#endif

//
// this test was added to check for bug reported on 21 May 2003:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_polymorphic<poly_bug>::value, true);

TT_TEST_END










