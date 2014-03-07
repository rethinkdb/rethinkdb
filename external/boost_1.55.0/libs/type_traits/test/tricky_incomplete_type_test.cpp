
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits.hpp>
#endif

TT_TEST_BEGIN(tricky_incomplete_type_test)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<incomplete_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_compound<incomplete_type>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_enum<incomplete_type>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_scalar<incomplete_type>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<int[]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<int[][3]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<const int[]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<const int[][3]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<volatile int[]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<volatile int[][3]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<const volatile int[]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_array<const volatile int[][3]>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_function_pointer<incomplete_type>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_function<incomplete_type>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pointer<incomplete_type>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_pointer<incomplete_type>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_member_object_pointer<incomplete_type>::value, false);

TT_TEST_END


