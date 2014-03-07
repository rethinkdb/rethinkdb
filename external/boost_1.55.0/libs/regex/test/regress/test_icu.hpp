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
  *   FILE         test_icu.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: MFC/ATL test handlers.
  */

#ifndef TEST_ICU_HPP
#define TEST_ICU_HPP

template <class charT, class Tag>
void test_icu(const charT&, const Tag&)
{
   // do nothing
}

void test_icu(const wchar_t&, const test_regex_search_tag&);
void test_icu(const wchar_t&, const test_invalid_regex_tag&);
void test_icu(const wchar_t&, const test_regex_replace_tag&);


#endif
