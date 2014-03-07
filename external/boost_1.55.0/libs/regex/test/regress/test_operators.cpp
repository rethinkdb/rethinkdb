/*
 *
 * Copyright (c) 2004
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include "test.hpp"

#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1300)\
   && !BOOST_WORKAROUND(__HP_aCC, BOOST_TESTED_AT(55500))\
   && !(defined(__GNUC__) && (__GNUC__ < 3) && !(defined(__SGI_STL_PORT) || defined(_STLPORT_VERSION)))

template <class T1, class T2>
void test_less(const T1& t1, const T2& t2)
{
   if(!(t1 < t2))
   {
      BOOST_REGEX_TEST_ERROR("Failed < comparison", char);
   }
   if(!(t1 <= t2))
   {
      BOOST_REGEX_TEST_ERROR("Failed <= comparison", char);
   }
   if(!(t1 != t2))
   {
      BOOST_REGEX_TEST_ERROR("Failed != comparison", char);
   }
   if(t1 == t2)
   {
      BOOST_REGEX_TEST_ERROR("Failed == comparison", char);
   }
   if(t1 >= t2)
   {
      BOOST_REGEX_TEST_ERROR("Failed >= comparison", char);
   }
   if(t1 > t2)
   {
      BOOST_REGEX_TEST_ERROR("Failed > comparison", char);
   }
}

template <class T1, class T2>
void test_greater(const T1& t1, const T2& t2)
{
   if(t1 < t2)
   {
      BOOST_REGEX_TEST_ERROR("Failed < comparison", char);
   }
   if(t1 <= t2)
   {
      BOOST_REGEX_TEST_ERROR("Failed <= comparison", char);
   }
   if(!(t1 != t2))
   {
      BOOST_REGEX_TEST_ERROR("Failed != comparison", char);
   }
   if(t1 == t2)
   {
      BOOST_REGEX_TEST_ERROR("Failed == comparison", char);
   }
   if(!(t1 >= t2))
   {
      BOOST_REGEX_TEST_ERROR("Failed >= comparison", char);
   }
   if(!(t1 > t2))
   {
      BOOST_REGEX_TEST_ERROR("Failed > comparison", char);
   }
}

template <class T1, class T2>
void test_equal(const T1& t1, const T2& t2)
{
   if(t1 < t2)
   {
      BOOST_REGEX_TEST_ERROR("Failed < comparison", char);
   }
   if(!(t1 <= t2))
   {
      BOOST_REGEX_TEST_ERROR("Failed <= comparison", char);
   }
   if(t1 != t2)
   {
      BOOST_REGEX_TEST_ERROR("Failed != comparison", char);
   }
   if(!(t1 == t2))
   {
      BOOST_REGEX_TEST_ERROR("Failed == comparison", char);
   }
   if(!(t1 >= t2))
   {
      BOOST_REGEX_TEST_ERROR("Failed >= comparison", char);
   }
   if(t1 > t2)
   {
      BOOST_REGEX_TEST_ERROR("Failed > comparison", char);
   }
}

template <class T1, class T2, class T3>
void test_plus(const T1& t1, const T2& t2, const T3& t3)
{
   if(t1 + t2 != t3)
   {
      BOOST_REGEX_TEST_ERROR("Failed addition", char);
   }
   if(t3 != t1 + t2)
   {
      BOOST_REGEX_TEST_ERROR("Failed addition", char);
   }
}

void test_operators()
{
   test_info<char>::set_typename("sub_match operators");

   std::string s1("a");
   std::string s2("b");
   boost::sub_match<std::string::const_iterator> sub1, sub2;
   sub1.first = s1.begin();
   sub1.second = s1.end();
   sub1.matched = true;
   sub2.first = s2.begin();
   sub2.second = s2.end();
   sub2.matched = true;

   test_less(sub1, sub2);
   test_less(sub1, s2.c_str());
   test_less(s1.c_str(), sub2);
   test_less(sub1, *s2.c_str());
   test_less(*s1.c_str(), sub2);
   test_less(sub1, s2);
   test_less(s1, sub2);
   test_greater(sub2, sub1);
   test_greater(sub2, s1.c_str());
   test_greater(s2.c_str(), sub1);
   test_greater(sub2, *s1.c_str());
   test_greater(*s2.c_str(), sub1);
   test_greater(sub2, s1);
   test_greater(s2, sub1);
   test_equal(sub1, sub1);
   test_equal(sub1, s1.c_str());
   test_equal(s1.c_str(), sub1);
   test_equal(sub1, *s1.c_str());
   test_equal(*s1.c_str(), sub1);
   test_equal(sub1, s1);
   test_equal(s1, sub1);
   test_plus(sub2, sub1, "ba");
   test_plus(sub2, s1.c_str(), "ba");
   test_plus(s2.c_str(), sub1, "ba");
   test_plus(sub2, *s1.c_str(), "ba");
   test_plus(*s2.c_str(), sub1, "ba");
   test_plus(sub2, s1, "ba");
   test_plus(s2, sub1, "ba");
}

#else

#include <iostream>

void test_operators()
{
   std::cout <<
   "\n<note>\n"
   "This compiler version does not support the sub_match comparison operators\n"
   "tests for these operators are not carried out\n"
   "</note>\n";
}

#endif

