
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_enum.hpp>
#endif

#ifndef BOOST_NO_CXX11_SCOPED_ENUMS

enum class test_enum
{
   a, b
};

#endif

TT_TEST_BEGIN(is_enum)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_enum<int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_enum<enum_UDT>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_enum<int_convertible>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_enum<int&>::value, false);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_enum<int&&>::value, false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_enum<boost::noncopyable>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_enum<void>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_enum<test_abc1>::value, false);
#ifndef BOOST_NO_CXX11_SCOPED_ENUMS
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_enum<test_enum>::value, true);
#endif

TT_TEST_END








