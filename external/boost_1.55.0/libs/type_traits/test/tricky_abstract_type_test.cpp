
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
#  include <boost/type_traits/is_stateless.hpp>
#endif

TT_TEST_BEGIN(tricky_abstract_type_test)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_empty<test_abc1>::value, false);
#ifndef TEST_STD
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_stateless<test_abc1>::value, false);
#endif

TT_TEST_END








