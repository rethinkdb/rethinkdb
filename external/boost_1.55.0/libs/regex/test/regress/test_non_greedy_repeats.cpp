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

#ifdef BOOST_MSVC
#pragma warning(disable:4127)
#endif

void test_non_greedy_repeats()
{
   //
   // non-greedy repeats added 21/04/00
   //
   using namespace boost::regex_constants;
   TEST_REGEX_SEARCH("a*?", perl, "aa", match_default, make_array(0, 0, -2, 0, 1, -2, 1, 1, -2, 1, 2, -2, 2, 2, -2, -2));
   TEST_REGEX_SEARCH("^a*?$", perl, "aa", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("^.*?$", perl, "aa", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("^(a)*?$", perl, "aa", match_default, make_array(0, 2, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("^[ab]*?$", perl, "aa", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("a??", perl, "aa", match_default, make_array(0, 0, -2, 0, 1, -2, 1, 1, -2, 1, 2, -2, 2, 2, -2, -2));
   TEST_REGEX_SEARCH("a+?", perl, "aa", match_default, make_array(0, 1, -2, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a{1,3}?", perl, "aaa", match_default, make_array(0, 1, -2, 1, 2, -2, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("\\w+?w", perl, "...ccccccwcccccw", match_default, make_array(3, 10, -2, 10, 16, -2, -2));
   TEST_REGEX_SEARCH("\\W+\\w+?w", perl, "...ccccccwcccccw", match_default, make_array(0, 10, -2, -2));
   TEST_REGEX_SEARCH("abc|\\w+?", perl, "abd", match_default, make_array(0, 1, -2, 1, 2, -2, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("abc|\\w+?", perl, "abcd", match_default, make_array(0, 3, -2, 3, 4, -2, -2));
   TEST_REGEX_SEARCH("<\\s*tag[^>]*>(.*?)<\\s*/tag\\s*>", perl, " <tag>here is some text</tag> <tag></tag>", match_default, make_array(1, 29, 6, 23, -2, 30, 41, 35, 35, -2, -2));
   TEST_REGEX_SEARCH("<\\s*tag[^>]*>(.*?)<\\s*/tag\\s*>", perl, " < tag attr=\"something\">here is some text< /tag > <tag></tag>", match_default, make_array(1, 49, 24, 41, -2, 50, 61, 55, 55, -2, -2));
   TEST_REGEX_SEARCH("xx-{0,2}?(?:[+-][0-9])??\\z", perl, "xx--", match_default, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("xx.{0,2}?(?:[+-][0-9])??\\z", perl, "xx--", match_default, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("xx.{0,2}?(?:[+-][0-9])??\\z", perl, "xx--", match_default|match_not_dot_newline, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("xx[/-]{0,2}?(?:[+-][0-9])??\\z", perl, "xx--", match_default, make_array(0, 4, -2, -2));
   TEST_INVALID_REGEX("a{1,3}{1}", perl);
   TEST_INVALID_REGEX("a**", perl);
}

