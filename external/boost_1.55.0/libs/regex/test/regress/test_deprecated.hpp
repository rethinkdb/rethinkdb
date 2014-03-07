/*
 *
 * Copyright (c) 1998-2004
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         test_deprecated.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Forward declare deprecated test functions.
  */

#ifndef BOOST_REGEX_TEST_DEPRECATED
#define BOOST_REGEX_TEST_DEPRECATED

template <class charT, class Tag>
void test_deprecated(const charT&, const Tag&)
{
   // do nothing
}

void test_deprecated(const char&, const test_regex_search_tag&);
void test_deprecated(const wchar_t&, const test_regex_search_tag&);
void test_deprecated(const char&, const test_invalid_regex_tag&);
void test_deprecated(const wchar_t&, const test_invalid_regex_tag&);


#endif

