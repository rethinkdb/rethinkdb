
//  (C) Copyright John Maddock 2005. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.tt.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_type.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/remove_all_extents.hpp>
#endif

BOOST_DECL_TRANSFORM_TEST(remove_all_extents_test_1, ::tt::remove_all_extents, const, const)
BOOST_DECL_TRANSFORM_TEST(remove_all_extents_test_2, ::tt::remove_all_extents, volatile, volatile)
BOOST_DECL_TRANSFORM_TEST3(remove_all_extents_test_3, ::tt::remove_all_extents, [2])
BOOST_DECL_TRANSFORM_TEST0(remove_all_extents_test_4, ::tt::remove_all_extents)
BOOST_DECL_TRANSFORM_TEST(remove_all_extents_test_5, ::tt::remove_all_extents, const &, const&)
BOOST_DECL_TRANSFORM_TEST(remove_all_extents_test_6, ::tt::remove_all_extents, *, *)
BOOST_DECL_TRANSFORM_TEST(remove_all_extents_test_7, ::tt::remove_all_extents, *volatile, *volatile)
BOOST_DECL_TRANSFORM_TEST(remove_all_extents_test_8, ::tt::remove_all_extents, const [2], const)
BOOST_DECL_TRANSFORM_TEST(remove_all_extents_test_9, ::tt::remove_all_extents, const &, const&)
BOOST_DECL_TRANSFORM_TEST(remove_all_extents_test_10, ::tt::remove_all_extents, const*, const*)
BOOST_DECL_TRANSFORM_TEST(remove_all_extents_test_11, ::tt::remove_all_extents, volatile*, volatile*)
BOOST_DECL_TRANSFORM_TEST(remove_all_extents_test_12, ::tt::remove_all_extents, const[2][3], const)
BOOST_DECL_TRANSFORM_TEST(remove_all_extents_test_13, ::tt::remove_all_extents, (&)[2], (&)[2])
BOOST_DECL_TRANSFORM_TEST3(remove_all_extents_test_14, ::tt::remove_all_extents, [])
BOOST_DECL_TRANSFORM_TEST(remove_all_extents_test_15, ::tt::remove_all_extents, const [], const)
BOOST_DECL_TRANSFORM_TEST(remove_all_extents_test_16, ::tt::remove_all_extents, const[][3], const)
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_DECL_TRANSFORM_TEST(remove_all_extents_test_5a, ::tt::remove_all_extents, const &&, const&&)
BOOST_DECL_TRANSFORM_TEST(remove_all_extents_test_13a, ::tt::remove_all_extents, (&&)[2], (&&)[2])
#endif

TT_TEST_BEGIN(remove_all_extents)

   remove_all_extents_test_1();
   remove_all_extents_test_2();
   remove_all_extents_test_3();
   remove_all_extents_test_4();
   remove_all_extents_test_5();
   remove_all_extents_test_6();
   remove_all_extents_test_7();
   remove_all_extents_test_8();
   remove_all_extents_test_9();
   remove_all_extents_test_10();
   remove_all_extents_test_11();
   remove_all_extents_test_12();
   remove_all_extents_test_13();
   remove_all_extents_test_14();
   remove_all_extents_test_15();
   remove_all_extents_test_16();
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
   remove_all_extents_test_5a();
   remove_all_extents_test_13a();
#endif

TT_TEST_END








