
//  (C) Copyright John Maddock 2010. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/conditional.hpp>
#endif
#include <boost/type_traits/is_same.hpp>

TT_TEST_BEGIN(conditional)

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same< ::tt::conditional<true, int, long>::type, int>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same< ::tt::conditional<false, int, long>::type, long>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same< ::tt::conditional<true, int, long>::type, long>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same< ::tt::conditional<false, int, long>::type, int>::value), false);

TT_TEST_END








