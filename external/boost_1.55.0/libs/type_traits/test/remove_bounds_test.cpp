
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.tt.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_type.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/remove_bounds.hpp>
#endif

BOOST_DECL_TRANSFORM_TEST(remove_bounds_test_1, ::tt::remove_bounds, const, const)
BOOST_DECL_TRANSFORM_TEST(remove_bounds_test_2, ::tt::remove_bounds, volatile, volatile)
BOOST_DECL_TRANSFORM_TEST3(remove_bounds_test_3, ::tt::remove_bounds, [2])
BOOST_DECL_TRANSFORM_TEST0(remove_bounds_test_4, ::tt::remove_bounds)
BOOST_DECL_TRANSFORM_TEST(remove_bounds_test_5, ::tt::remove_bounds, const &, const&)
BOOST_DECL_TRANSFORM_TEST(remove_bounds_test_6, ::tt::remove_bounds, *, *)
BOOST_DECL_TRANSFORM_TEST(remove_bounds_test_7, ::tt::remove_bounds, *volatile, *volatile)
BOOST_DECL_TRANSFORM_TEST(remove_bounds_test_8, ::tt::remove_bounds, const [2], const)
BOOST_DECL_TRANSFORM_TEST(remove_bounds_test_9, ::tt::remove_bounds, const &, const&)
BOOST_DECL_TRANSFORM_TEST(remove_bounds_test_10, ::tt::remove_bounds, const*, const*)
BOOST_DECL_TRANSFORM_TEST(remove_bounds_test_11, ::tt::remove_bounds, volatile*, volatile*)
BOOST_DECL_TRANSFORM_TEST(remove_bounds_test_12, ::tt::remove_bounds, const[2][3], const[3])
BOOST_DECL_TRANSFORM_TEST(remove_bounds_test_13, ::tt::remove_bounds, (&)[2], (&)[2])
BOOST_DECL_TRANSFORM_TEST3(remove_bounds_test_14, ::tt::remove_bounds, [])
BOOST_DECL_TRANSFORM_TEST(remove_bounds_test_15, ::tt::remove_bounds, const [], const)
BOOST_DECL_TRANSFORM_TEST(remove_bounds_test_16, ::tt::remove_bounds, const[][3], const[3])
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_DECL_TRANSFORM_TEST(remove_bounds_test_5a, ::tt::remove_bounds, const &&, const&&)
BOOST_DECL_TRANSFORM_TEST(remove_bounds_test_13a, ::tt::remove_bounds, (&&)[2], (&&)[2])
#endif

TT_TEST_BEGIN(remove_bounds)

   remove_bounds_test_1();
   remove_bounds_test_2();
   remove_bounds_test_3();
   remove_bounds_test_4();
   remove_bounds_test_5();
   remove_bounds_test_6();
   remove_bounds_test_7();
   remove_bounds_test_8();
   remove_bounds_test_9();
   remove_bounds_test_10();
   remove_bounds_test_11();
   remove_bounds_test_12();
   remove_bounds_test_13();
   remove_bounds_test_14();
   remove_bounds_test_15();
   remove_bounds_test_16();
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
   remove_bounds_test_5a();
   remove_bounds_test_13a();
#endif

TT_TEST_END








