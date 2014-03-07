
//  (C) Copyright John Maddock 2000. 
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

BOOST_DECL_TRANSFORM_TEST(add_pointer_test_1, ::tt::add_pointer, const, const*)
BOOST_DECL_TRANSFORM_TEST(add_pointer_test_2, ::tt::add_pointer, volatile, volatile*)
BOOST_DECL_TRANSFORM_TEST(add_pointer_test_3, ::tt::add_pointer, *, **)
BOOST_DECL_TRANSFORM_TEST2(add_pointer_test_4, ::tt::add_pointer, *)
BOOST_DECL_TRANSFORM_TEST(add_pointer_test_7, ::tt::add_pointer, *volatile, *volatile*)
BOOST_DECL_TRANSFORM_TEST(add_pointer_test_10, ::tt::add_pointer, const*, const**)
BOOST_DECL_TRANSFORM_TEST(add_pointer_test_11, ::tt::add_pointer, volatile*, volatile**)

TT_TEST_BEGIN(add_pointer)

   add_pointer_test_1();
   add_pointer_test_2();
   add_pointer_test_3();
   add_pointer_test_4();
   add_pointer_test_7();
   add_pointer_test_10();
   add_pointer_test_11();

TT_TEST_END








