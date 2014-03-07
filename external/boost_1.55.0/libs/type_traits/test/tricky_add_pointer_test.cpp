
//  (C) Copyright John Maddock 2002. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.tt.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_type.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/add_pointer.hpp>
#endif

BOOST_DECL_TRANSFORM_TEST(add_pointer_test_5, ::tt::add_pointer, const &, const*)
BOOST_DECL_TRANSFORM_TEST(add_pointer_test_6, ::tt::add_pointer, &, *)
BOOST_DECL_TRANSFORM_TEST(add_pointer_test_8, ::tt::add_pointer, const [2], const (*)[2])
BOOST_DECL_TRANSFORM_TEST(add_pointer_test_9, ::tt::add_pointer, const &, const*)
BOOST_DECL_TRANSFORM_TEST(add_pointer_test_12, ::tt::add_pointer, const[2][3], const (*)[2][3])
BOOST_DECL_TRANSFORM_TEST(add_pointer_test_13, ::tt::add_pointer, (&)[2], (*)[2])
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_DECL_TRANSFORM_TEST(add_pointer_test_5a, ::tt::add_pointer, const &&, const*)
BOOST_DECL_TRANSFORM_TEST(add_pointer_test_6a, ::tt::add_pointer, &&, *)
BOOST_DECL_TRANSFORM_TEST(add_pointer_test_9a, ::tt::add_pointer, const &&, const*)
BOOST_DECL_TRANSFORM_TEST(add_pointer_test_13a, ::tt::add_pointer, (&&)[2], (*)[2])
#endif

TT_TEST_BEGIN(tricky_add_pointer_test)

   add_pointer_test_5();
   add_pointer_test_6();
   add_pointer_test_8();
   add_pointer_test_9();
   add_pointer_test_12();
   add_pointer_test_13();
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
   add_pointer_test_5a();
   add_pointer_test_6a();
   add_pointer_test_9a();
   add_pointer_test_13a();
#endif

TT_TEST_END








