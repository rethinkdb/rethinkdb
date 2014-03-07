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

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         test_mfc.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: MFC/ATL test handlers.
  */

#ifndef TEST_MFC_HPP
#define TEST_MFC_HPP

template <class charT, class Tag>
void test_mfc(const charT&, const Tag&)
{
   // do nothing
}

void test_mfc(const char&, const test_regex_search_tag&);
void test_mfc(const wchar_t&, const test_regex_search_tag&);
void test_mfc(const char&, const test_invalid_regex_tag&);
void test_mfc(const wchar_t&, const test_invalid_regex_tag&);
void test_mfc(const char&, const test_regex_replace_tag&);
void test_mfc(const wchar_t&, const test_regex_replace_tag&);


#endif
