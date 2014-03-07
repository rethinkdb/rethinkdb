
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.tt.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_type.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/remove_volatile.hpp>
#endif

BOOST_DECL_TRANSFORM_TEST(remove_volatile_test_1, ::tt::remove_volatile, const, const)
BOOST_DECL_TRANSFORM_TEST3(remove_volatile_test_2, ::tt::remove_volatile, volatile)
BOOST_DECL_TRANSFORM_TEST(remove_volatile_test_3, ::tt::remove_volatile, const volatile, const)
BOOST_DECL_TRANSFORM_TEST0(remove_volatile_test_4, ::tt::remove_volatile)
BOOST_DECL_TRANSFORM_TEST0(remove_volatile_test_5, ::tt::remove_volatile)
BOOST_DECL_TRANSFORM_TEST(remove_volatile_test_6, ::tt::remove_volatile, *const, *const)
BOOST_DECL_TRANSFORM_TEST(remove_volatile_test_7, ::tt::remove_volatile, *volatile, *)
BOOST_DECL_TRANSFORM_TEST(remove_volatile_test_8, ::tt::remove_volatile, *const volatile, *const)
BOOST_DECL_TRANSFORM_TEST(remove_volatile_test_9, ::tt::remove_volatile, *, *)
BOOST_DECL_TRANSFORM_TEST(remove_volatile_test_10, ::tt::remove_volatile, *, *)
BOOST_DECL_TRANSFORM_TEST(remove_volatile_test_11, ::tt::remove_volatile, volatile*, volatile*)
BOOST_DECL_TRANSFORM_TEST(remove_volatile_test_12, ::tt::remove_volatile, const[2], const[2])
BOOST_DECL_TRANSFORM_TEST(remove_volatile_test_13, ::tt::remove_volatile, volatile[2], [2])
BOOST_DECL_TRANSFORM_TEST(remove_volatile_test_14, ::tt::remove_volatile, const volatile[2], const[2])
BOOST_DECL_TRANSFORM_TEST(remove_volatile_test_15, ::tt::remove_volatile, [2], [2])
BOOST_DECL_TRANSFORM_TEST(remove_volatile_test_16, ::tt::remove_volatile, const*, const*)
BOOST_DECL_TRANSFORM_TEST(remove_volatile_test_17, ::tt::remove_volatile, const*volatile, const*)

TT_TEST_BEGIN(remove_volatile)

   remove_volatile_test_1();
   remove_volatile_test_2();
   remove_volatile_test_3();
   remove_volatile_test_4();
   remove_volatile_test_5();
   remove_volatile_test_6();
   remove_volatile_test_7();
   remove_volatile_test_8();
   remove_volatile_test_9();
   remove_volatile_test_10();
   remove_volatile_test_11();
   remove_volatile_test_12();
   remove_volatile_test_13();
   remove_volatile_test_14();
   remove_volatile_test_15();
   remove_volatile_test_16();
   remove_volatile_test_17();

TT_TEST_END








