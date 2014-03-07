
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_compound.hpp>
#endif

TT_TEST_BEGIN(is_compound)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_compound<UDT>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_compound<void*>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_compound<void>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_compound<int>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_compound<test_abc1>::value, true);

TT_TEST_END








