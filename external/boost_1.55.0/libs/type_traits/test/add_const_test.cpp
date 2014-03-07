
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.tt.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_type.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/add_const.hpp>
#endif

BOOST_DECL_TRANSFORM_TEST(add_const_test_1, ::tt::add_const, const, const)
BOOST_DECL_TRANSFORM_TEST(add_const_test_2, ::tt::add_const, volatile, volatile const)
BOOST_DECL_TRANSFORM_TEST(add_const_test_3, ::tt::add_const, *, *const)
BOOST_DECL_TRANSFORM_TEST2(add_const_test_4, ::tt::add_const, const)
BOOST_DECL_TRANSFORM_TEST(add_const_test_7, ::tt::add_const, *volatile, *volatile const)
BOOST_DECL_TRANSFORM_TEST(add_const_test_10, ::tt::add_const, const*, const*const)
BOOST_DECL_TRANSFORM_TEST(add_const_test_11, ::tt::add_const, volatile*, volatile*const)
BOOST_DECL_TRANSFORM_TEST(add_const_test_5, ::tt::add_const, const &, const&)
BOOST_DECL_TRANSFORM_TEST(add_const_test_6, ::tt::add_const, &, &)
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_DECL_TRANSFORM_TEST(add_const_test_5a, ::tt::add_const, const &&, const&&)
BOOST_DECL_TRANSFORM_TEST(add_const_test_6a, ::tt::add_const, &&, &&)
#endif
BOOST_DECL_TRANSFORM_TEST(add_const_test_8, ::tt::add_const, const [2], const [2])
BOOST_DECL_TRANSFORM_TEST(add_const_test_9, ::tt::add_const, volatile &, volatile&)
BOOST_DECL_TRANSFORM_TEST(add_const_test_12, ::tt::add_const, [2][3], const[2][3])
BOOST_DECL_TRANSFORM_TEST(add_const_test_13, ::tt::add_const, (&)[2], (&)[2])

TT_TEST_BEGIN(add_const)

   add_const_test_1();
   add_const_test_2();
   add_const_test_3();
   add_const_test_4();
   add_const_test_7();
   add_const_test_10();
   add_const_test_11();
   add_const_test_5();
   add_const_test_6();
   add_const_test_8();
   add_const_test_9();
   add_const_test_12();
   add_const_test_13();
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
   add_const_test_5a();
   add_const_test_6a();
#endif

TT_TEST_END








