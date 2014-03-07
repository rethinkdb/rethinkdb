
//  (C) Copyright John Maddock 2005. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_member_object_pointer.hpp>
#endif

typedef const double (UDT::*mp2) ;

TT_TEST_BEGIN(is_member_object_pointer)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<f1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<f2>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<f3>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<void*>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<mf1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<mf2>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<mf3>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<mf4>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<cmf>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<mp>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<mp2>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<void>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<test_abc1>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<foo0_t>::value, false);

TT_TEST_END








