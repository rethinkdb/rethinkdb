
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.tt.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_type.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/remove_cv.hpp>
#endif

BOOST_DECL_TRANSFORM_TEST3(remove_cv_test_1, ::tt::remove_cv, const)
BOOST_DECL_TRANSFORM_TEST3(remove_cv_test_2, ::tt::remove_cv, volatile)
BOOST_DECL_TRANSFORM_TEST3(remove_cv_test_3, ::tt::remove_cv, const volatile)
BOOST_DECL_TRANSFORM_TEST0(remove_cv_test_4, ::tt::remove_cv)
BOOST_DECL_TRANSFORM_TEST(remove_cv_test_5, ::tt::remove_cv, const &, const&)
BOOST_DECL_TRANSFORM_TEST(remove_cv_test_6, ::tt::remove_cv, *const, *)
BOOST_DECL_TRANSFORM_TEST(remove_cv_test_7, ::tt::remove_cv, *volatile, *)
BOOST_DECL_TRANSFORM_TEST(remove_cv_test_8, ::tt::remove_cv, *const volatile, *)
BOOST_DECL_TRANSFORM_TEST(remove_cv_test_9, ::tt::remove_cv, *, *)
BOOST_DECL_TRANSFORM_TEST(remove_cv_test_10, ::tt::remove_cv, const*, const*)
BOOST_DECL_TRANSFORM_TEST(remove_cv_test_11, ::tt::remove_cv, volatile*, volatile*)
BOOST_DECL_TRANSFORM_TEST(remove_cv_test_12, ::tt::remove_cv, const[2], [2])
BOOST_DECL_TRANSFORM_TEST(remove_cv_test_13, ::tt::remove_cv, volatile[2], [2])
BOOST_DECL_TRANSFORM_TEST(remove_cv_test_14, ::tt::remove_cv, const volatile[2], [2])
BOOST_DECL_TRANSFORM_TEST(remove_cv_test_15, ::tt::remove_cv, [2], [2])
BOOST_DECL_TRANSFORM_TEST(remove_cv_test_16, ::tt::remove_cv, const*, const*)
BOOST_DECL_TRANSFORM_TEST(remove_cv_test_17, ::tt::remove_cv, const*volatile, const*)
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_DECL_TRANSFORM_TEST(remove_cv_test_5a, ::tt::remove_cv, const &&, const&&)
#endif

TT_TEST_BEGIN(remove_cv)

   remove_cv_test_1();
   remove_cv_test_2();
   remove_cv_test_3();
   remove_cv_test_4();
   remove_cv_test_5();
   remove_cv_test_6();
   remove_cv_test_7();
   remove_cv_test_8();
   remove_cv_test_9();
   remove_cv_test_10();
   remove_cv_test_11();
   remove_cv_test_12();
   remove_cv_test_13();
   remove_cv_test_14();
   remove_cv_test_15();
   remove_cv_test_16();
   remove_cv_test_17();
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
   remove_cv_test_5a();
#endif

TT_TEST_END








