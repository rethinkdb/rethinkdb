
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.tt.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_type.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/remove_reference.hpp>
#endif

BOOST_DECL_TRANSFORM_TEST(remove_reference_test_1, ::tt::remove_reference, const, const)
BOOST_DECL_TRANSFORM_TEST(remove_reference_test_2, ::tt::remove_reference, volatile, volatile)
BOOST_DECL_TRANSFORM_TEST3(remove_reference_test_3, ::tt::remove_reference, &)
BOOST_DECL_TRANSFORM_TEST0(remove_reference_test_4, ::tt::remove_reference)
BOOST_DECL_TRANSFORM_TEST(remove_reference_test_5, ::tt::remove_reference, const &, const)
BOOST_DECL_TRANSFORM_TEST(remove_reference_test_6, ::tt::remove_reference, *, *)
BOOST_DECL_TRANSFORM_TEST(remove_reference_test_7, ::tt::remove_reference, *volatile, *volatile)
BOOST_DECL_TRANSFORM_TEST3(remove_reference_test_8, ::tt::remove_reference, &)
BOOST_DECL_TRANSFORM_TEST(remove_reference_test_9, ::tt::remove_reference, const &, const)
BOOST_DECL_TRANSFORM_TEST(remove_reference_test_10, ::tt::remove_reference, const*, const*)
BOOST_DECL_TRANSFORM_TEST(remove_reference_test_11, ::tt::remove_reference, volatile*, volatile*)
BOOST_DECL_TRANSFORM_TEST(remove_reference_test_12, ::tt::remove_reference, const[2], const[2])
BOOST_DECL_TRANSFORM_TEST(remove_reference_test_13, ::tt::remove_reference, (&)[2], [2])
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_DECL_TRANSFORM_TEST3(remove_reference_test_3a, ::tt::remove_reference, &&)
BOOST_DECL_TRANSFORM_TEST(remove_reference_test_5a, ::tt::remove_reference, const &&, const)
BOOST_DECL_TRANSFORM_TEST3(remove_reference_test_8a, ::tt::remove_reference, &&)
BOOST_DECL_TRANSFORM_TEST(remove_reference_test_9a, ::tt::remove_reference, const &&, const)
BOOST_DECL_TRANSFORM_TEST(remove_reference_test_13a, ::tt::remove_reference, (&&)[2], [2])
#endif

TT_TEST_BEGIN(remove_reference)

   remove_reference_test_1();
   remove_reference_test_2();
   remove_reference_test_3();
   remove_reference_test_4();
   remove_reference_test_5();
   remove_reference_test_6();
   remove_reference_test_7();
   remove_reference_test_8();
   remove_reference_test_9();
   remove_reference_test_10();
   remove_reference_test_11();
   remove_reference_test_12();
   remove_reference_test_13();
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
   remove_reference_test_3a();
   remove_reference_test_5a();
   remove_reference_test_8a();
   remove_reference_test_9a();
   remove_reference_test_13a();
#endif

TT_TEST_END








