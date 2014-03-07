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

void test_grep()
{
   //
   // now test grep,
   // basically check all our restart types - line, word, etc
   // checking each one for null and non-null matches.
   //
   using namespace boost::regex_constants;
   TEST_REGEX_SEARCH("a", perl|nosubs, " a a a aa", match_default, make_array(1, 2, -2, 3, 4, -2, 5, 6, -2, 7, 8, -2, 8, 9, -2, -2));
   TEST_REGEX_SEARCH("a+b+", perl|nosubs, "aabaabbb ab", match_default, make_array(0, 3, -2, 3, 8, -2, 9, 11, -2, -2));
   TEST_REGEX_SEARCH("a(b*|c|e)d", perl|nosubs, "adabbdacd", match_default, make_array(0, 2, -2, 2, 6, -2, 6, 9, -2, -2));
   TEST_REGEX_SEARCH("a", perl|nosubs, "\na\na\na\naa", match_default, make_array(1, 2, -2, 3, 4, -2, 5, 6, -2, 7, 8, -2, 8, 9, -2, -2));
   TEST_REGEX_SEARCH("^", perl|nosubs, "   \n\n  \n\n\n", match_default, make_array(0, 0, -2, 4, 4, -2, 5, 5, -2, 8, 8, -2, 9, 9, -2, 10, 10, -2, -2));
   TEST_REGEX_SEARCH("^ab", perl|nosubs, "ab  \nab  ab\n", match_default, make_array(0, 2, -2, 5, 7, -2, -2));
   TEST_REGEX_SEARCH("^[^\\n]*\n", perl|nosubs, "   \n  \n\n  \n", match_default, make_array(0, 4, -2, 4, 7, -2, 7, 8, -2, 8, 11, -2, -2));
   TEST_REGEX_SEARCH("\\<abc", perl|nosubs, "abcabc abc\n\nabc", match_default, make_array(0, 3, -2, 7, 10, -2, 12, 15, -2, -2));
   TEST_REGEX_SEARCH("\\<", perl|nosubs, "  ab a aaa  ", match_default, make_array(2, 2, -2, 5, 5, -2, 7, 7, -2, -2));
   TEST_REGEX_SEARCH("\\<\\w+\\W+", perl|nosubs, " aa  aa  a ", match_default, make_array(1, 5, -2, 5, 9, -2, 9, 11, -2, -2));

   TEST_REGEX_SEARCH("\\Aabc", perl|nosubs, "abc   abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("\\G\\w+\\W+", perl|nosubs, "abc  abc a cbbb   ", match_default, make_array(0, 5, -2, 5, 9, -2, 9, 11, -2, 11, 18, -2, -2));
   TEST_REGEX_SEARCH("\\Ga+b+", perl|nosubs, "aaababb  abb", match_default, make_array(0, 4, -2, 4, 7, -2, -2));
   TEST_REGEX_SEARCH("abc", perl|nosubs, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("abc", perl|nosubs, " abc abcabc", match_default, make_array(1, 4, -2, 5, 8, -2, 8, 11, -2, -2));
   TEST_REGEX_SEARCH("\\n\\n", perl|nosubs, " \n\n\n       \n      \n\n\n\n  ", match_default, make_array(1, 3, -2, 18, 20, -2, 20, 22, -2, -2));
   TEST_REGEX_SEARCH("$", perl|nosubs, "   \n\n  \n\n\n", match_default, make_array(3, 3, -2, 4, 4, -2, 7, 7, -2, 8, 8, -2, 9, 9, -2, 10, 10, -2, -2));
   TEST_REGEX_SEARCH("\\b", perl|nosubs, "  abb a abbb ", match_default, make_array(2, 2, -2, 5, 5, -2, 6, 6, -2, 7, 7, -2, 8, 8, -2, 12, 12, -2, -2));
   TEST_REGEX_SEARCH("A", perl|icase|nosubs, " a a a aa", match_default, make_array(1, 2, -2, 3, 4, -2, 5, 6, -2, 7, 8, -2, 8, 9, -2, -2));
   TEST_REGEX_SEARCH("A+B+", perl|icase|nosubs, "aabaabbb ab", match_default, make_array(0, 3, -2, 3, 8, -2, 9, 11, -2, -2));
   TEST_REGEX_SEARCH("A(B*|c|e)D", perl|icase|nosubs, "adabbdacd", match_default, make_array(0, 2, -2, 2, 6, -2, 6, 9, -2, -2));
   TEST_REGEX_SEARCH("A", perl|icase|nosubs, "\na\na\na\naa", match_default, make_array(1, 2, -2, 3, 4, -2, 5, 6, -2, 7, 8, -2, 8, 9, -2, -2));
   TEST_REGEX_SEARCH("^aB", perl|icase|nosubs, "Ab  \nab  Ab\n", match_default, make_array(0, 2, -2, 5, 7, -2, -2));
   TEST_REGEX_SEARCH("\\<abc", perl|icase|nosubs, "Abcabc aBc\n\nabc", match_default, make_array(0, 3, -2, 7, 10, -2, 12, 15, -2, -2));
   TEST_REGEX_SEARCH("ABC", perl|icase|nosubs, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("abc", perl|icase|nosubs, " ABC ABCABC ", match_default, make_array(1, 4, -2, 5, 8, -2, 8, 11, -2, -2));
   TEST_REGEX_SEARCH("a|\\Ab", perl, "b ab", match_default, make_array(0, 1, -2, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a|^b", perl, "b ab\nb", match_default, make_array(0, 1, -2, 2, 3, -2, 5, 6, -2, -2));
   TEST_REGEX_SEARCH("a|\\<b", perl, "b ab\nb", match_default, make_array(0, 1, -2, 2, 3, -2, 5, 6, -2, -2));

   TEST_REGEX_SEARCH("\\Aabc", perl, "abcabc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("^abc", perl, "abcabc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("\\<abc", perl, "abcabc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("\\babc", perl, "abcabc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("(?<=\\Aabc)?abc", perl, "abcabc", match_default, make_array(0, 3, -2, 3, 6, -2, -2));
   TEST_REGEX_SEARCH("(?<=^abc)?abc", perl, "abcabc", match_default, make_array(0, 3, -2, 3, 6, -2, -2));
   TEST_REGEX_SEARCH("(?<=\\<abc)?abc", perl, "abcabc", match_default, make_array(0, 3, -2, 3, 6, -2, -2));
   TEST_REGEX_SEARCH("(?<=\\babc)?abc", perl, "abcabc", match_default, make_array(0, 3, -2, 3, 6, -2, -2));
   TEST_REGEX_SEARCH("(?<=^).{2}|(?<=^.{3}).{2}", perl, "123456789", match_default, make_array(0, 2, -2, 3, 5, -2, -2));
}

