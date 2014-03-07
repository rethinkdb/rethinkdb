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

void test_emacs2();

void test_emacs()
{
   using namespace boost::regex_constants;
   // now try operator + :
   TEST_REGEX_SEARCH("ab+", emacs, "a", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("ab+", emacs, "ab", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("ab+", emacs, "sssabbbbbbsss", match_default, make_array(3, 10, -2, -2));
   TEST_REGEX_SEARCH("ab+c+", emacs, "abbb", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("ab+c+", emacs, "accc", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("ab+c+", emacs, "abbcc", match_default, make_array(0, 5, -2, -2));
   TEST_INVALID_REGEX("\\<+", emacs);
   TEST_INVALID_REGEX("\\>+", emacs);
   TEST_REGEX_SEARCH("\n+", emacs, "\n\n", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("\\+", emacs, "+", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\+", emacs, "++", match_default, make_array(0, 1, -2, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("\\++", emacs, "++", match_default, make_array(0, 2, -2, -2));

   // now try operator ?
   TEST_REGEX_SEARCH("a?", emacs, "b", match_default, make_array(0, 0, -2, 1, 1, -2, -2));
   TEST_REGEX_SEARCH("ab?", emacs, "a", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("ab?", emacs, "ab", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("ab?", emacs, "sssabbbbbbsss", match_default, make_array(3, 5, -2, -2));
   TEST_REGEX_SEARCH("ab?c?", emacs, "a", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("ab?c?", emacs, "abbb", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("ab?c?", emacs, "accc", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("ab?c?", emacs, "abcc", match_default, make_array(0, 3, -2, -2));
   TEST_INVALID_REGEX("\\<?", emacs);
   TEST_INVALID_REGEX("\\>?", emacs);
   TEST_REGEX_SEARCH("\n?", emacs, "\n\n", match_default, make_array(0, 1, -2, 1, 2, -2, 2, 2, -2, -2));
   TEST_REGEX_SEARCH("\\?", emacs, "?", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\?", emacs, "?", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\??", emacs, "??", match_default, make_array(0, 1, -2, 1, 2, -2, 2, 2, -2, -2));

   TEST_REGEX_SEARCH("a*?", emacs, "aa", match_default, make_array(0, 0, -2, 0, 1, -2, 1, 1, -2, 1, 2, -2, 2, 2, -2, -2));
   TEST_REGEX_SEARCH("^a*?$", emacs, "aa", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("^.*?$", emacs, "aa", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("^\\(a\\)*?$", emacs, "aa", match_default, make_array(0, 2, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("^[ab]*?$", emacs, "aa", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("a??", emacs, "aa", match_default, make_array(0, 0, -2, 0, 1, -2, 1, 1, -2, 1, 2, -2, 2, 2, -2, -2));
   TEST_REGEX_SEARCH("a+?", emacs, "aa", match_default, make_array(0, 1, -2, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a\\{1,3\\}?", emacs, "aaa", match_default, make_array(0, 1, -2, 1, 2, -2, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("\\w+?w", emacs, "...ccccccwcccccw", match_default, make_array(3, 10, -2, 10, 16, -2, -2));
   TEST_REGEX_SEARCH("\\W+\\w+?w", emacs, "...ccccccwcccccw", match_default, make_array(0, 10, -2, -2));
   TEST_REGEX_SEARCH("abc\\|\\w+?", emacs, "abd", match_default, make_array(0, 1, -2, 1, 2, -2, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("abc\\|\\w+?", emacs, "abcd", match_default, make_array(0, 3, -2, 3, 4, -2, -2));
   TEST_REGEX_SEARCH("<\\ss*tag[^>]*>\\(.*?\\)<\\ss*/tag\\ss*>", emacs, " <tag>here is some text</tag> <tag></tag>", match_default, make_array(1, 29, 6, 23, -2, 30, 41, 35, 35, -2, -2));
   TEST_REGEX_SEARCH("<\\ss*tag[^>]*>\\(.*?\\)<\\ss*/tag\\ss*>", emacs, " < tag attr=\"something\">here is some text< /tag > <tag></tag>", match_default, make_array(1, 49, 24, 41, -2, 50, 61, 55, 55, -2, -2));
   TEST_INVALID_REGEX("a\\{1,3\\}\\{1\\}", emacs);
   TEST_INVALID_REGEX("a**", emacs);
   TEST_INVALID_REGEX("a++", emacs);

   TEST_REGEX_SEARCH("\\<abcd", emacs, "  abcd", match_default, make_array(2, 6, -2, -2));
   TEST_REGEX_SEARCH("\\<ab", emacs, "cab", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("\\<ab", emacs, "\nab", match_default, make_array(1, 3, -2, -2));
   TEST_REGEX_SEARCH("\\<tag", emacs, "::tag", match_default, make_array(2, 5, -2, -2));
   TEST_REGEX_SEARCH("\\<abcd", emacs, "abcd", match_default|match_not_bow, make_array(-2, -2));
   TEST_REGEX_SEARCH("\\<abcd", emacs, "  abcd", match_default|match_not_bow, make_array(2, 6, -2, -2));
   TEST_REGEX_SEARCH("\\<", emacs, "ab ", match_default|match_not_bow, make_array(-2, -2));
   TEST_REGEX_SEARCH(".\\<.", emacs, "ab", match_default|match_not_bow, make_array(-2, -2));
   TEST_REGEX_SEARCH(".\\<.", emacs, " b", match_default|match_not_bow, make_array(0, 2, -2, -2));
   // word end:
   TEST_REGEX_SEARCH("abc\\>", emacs, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("abc\\>", emacs, "abcd", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("abc\\>", emacs, "abc\n", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("abc\\>", emacs, "abc::", match_default, make_array(0,3, -2, -2));
   TEST_REGEX_SEARCH("abc\\(?:\\>..\\|$\\)", emacs, "abc::", match_default, make_array(0, 5, -2, -2));
   TEST_REGEX_SEARCH("\\>", emacs, "  ", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH(".\\>.", emacs, "  ", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("abc\\>", emacs, "abc", match_default|match_not_eow, make_array(-2, -2));
   // word boundary:
   TEST_REGEX_SEARCH("\\babcd", emacs, "  abcd", match_default, make_array(2, 6, -2, -2));
   TEST_REGEX_SEARCH("\\bab", emacs, "cab", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("\\bab", emacs, "\nab", match_default, make_array(1, 3, -2, -2));
   TEST_REGEX_SEARCH("\\btag", emacs, "::tag", match_default, make_array(2, 5, -2, -2));
   TEST_REGEX_SEARCH("abc\\b", emacs, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("abc\\b", emacs, "abcd", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("abc\\b", emacs, "abc\n", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("abc\\b", emacs, "abc::", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("\\babcd", emacs, "abcd", match_default|match_not_bow, make_array(-2, -2));
   // within word:
   TEST_REGEX_SEARCH("\\B", emacs, "ab", match_default, make_array(1, 1, -2, -2));
   TEST_REGEX_SEARCH("a\\Bb", emacs, "ab", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("a\\B", emacs, "ab", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("a\\B", emacs, "a", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("a\\B", emacs, "a ", match_default, make_array(-2, -2));
   // buffer operators:
   TEST_REGEX_SEARCH("\\`abc", emacs, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("\\`abc", emacs, "\nabc", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("\\`abc", emacs, " abc", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("abc\\'", emacs, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("abc\\'", emacs, "abc\n", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("abc\\'", emacs, "abc ", match_default, make_array(-2, -2));

   TEST_REGEX_SEARCH("a\\|b", emacs, "a", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("a\\|b", emacs, "b", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("a\\|b\\|c", emacs, "c", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("a\\|\\(b\\)\\|.", emacs, "b", match_default, make_array(0, 1, 0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\(a\\)\\|b\\|.", emacs, "a", match_default, make_array(0, 1, 0, 1, -2, -2));
   TEST_REGEX_SEARCH("a\\(b\\|c\\)", emacs, "ab", match_default, make_array(0, 2, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a\\(b\\|c\\)", emacs, "ac", match_default, make_array(0, 2, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a\\(b\\|c\\)", emacs, "ad", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("\\(a\\|b\\|c\\)", emacs, "c", match_default, make_array(0, 1, 0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\(a\\|\\(b\\)\\|.\\)", emacs, "b", match_default, make_array(0, 1, 0, 1, 0, 1, -2, -2));
   TEST_INVALID_REGEX("\\|c", emacs);
   TEST_INVALID_REGEX("c\\|", emacs);
   TEST_INVALID_REGEX("\\(\\|\\)", emacs);
   TEST_INVALID_REGEX("\\(a\\|\\)", emacs);
   TEST_INVALID_REGEX("\\(\\|a\\)", emacs);

   TEST_REGEX_SEARCH("\\(?:abc\\)+", emacs, "xxabcabcxx", match_default, make_array(2, 8, -2, -2));
   TEST_REGEX_SEARCH("\\(?:a+\\)\\(b+\\)", emacs, "xaaabbbx", match_default, make_array(1, 7, 4, 7, -2, -2));
   TEST_REGEX_SEARCH("\\(a+\\)\\(?:b+\\)", emacs, "xaaabbba", match_default, make_array(1, 7, 1, 4, -2, -2));
   TEST_REGEX_SEARCH("\\(?:\\(a+\\)b+\\)", emacs, "xaaabbba", match_default, make_array(1, 7, 1, 4, -2, -2));
   TEST_REGEX_SEARCH("\\(?:a+\\(b+\\)\\)", emacs, "xaaabbba", match_default, make_array(1, 7, 4, 7, -2, -2));
   TEST_REGEX_SEARCH("a+\\(?#b+\\)b+", emacs, "xaaabbba", match_default, make_array(1, 7, -2, -2));
   TEST_REGEX_SEARCH("\\(a\\)\\(?:b\\|$\\)", emacs, "ab", match_default, make_array(0, 2, 0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\(a\\)\\(?:b\\|$\\)", emacs, "a", match_default, make_array(0, 1, 0, 1, -2, -2));
   test_emacs2();
}

void test_emacs2()
{
   using namespace boost::regex_constants;


   TEST_REGEX_SEARCH("\\ss+", emacs, "a  b", match_default, make_array(1, 3, -2, -2));
   TEST_REGEX_SEARCH("\\Ss+", emacs, " ab ", match_default, make_array(1, 3, -2, -2));
   TEST_REGEX_SEARCH("\\sw+", emacs, " ab ", match_default, make_array(1, 3, -2, -2));
   TEST_REGEX_SEARCH("\\Sw+", emacs, "a  b", match_default, make_array(1, 3, -2, -2));
   TEST_REGEX_SEARCH("\\s_+", emacs, " $&*+-_<> ", match_default, make_array(1, 9, -2, -2));
   TEST_REGEX_SEARCH("\\S_+", emacs, "$&*+-_<>b", match_default, make_array(8, 9, -2, -2));
   TEST_REGEX_SEARCH("\\s.+", emacs, " .,;!? ", match_default, make_array(1, 6, -2, -2));
   TEST_REGEX_SEARCH("\\S.+", emacs, ".,;!?b", match_default, make_array(5, 6, -2, -2));
   TEST_REGEX_SEARCH("\\s(+", emacs, "([{ ", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("\\S(+", emacs, "([{ ", match_default, make_array(3, 4, -2, -2));
   TEST_REGEX_SEARCH("\\s)+", emacs, ")]} ", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("\\S)+", emacs, ")]} ", match_default, make_array(3, 4, -2, -2));
   TEST_REGEX_SEARCH("\\s\"+", emacs, "\"'` ", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("\\S\"+", emacs, "\"'` ", match_default, make_array(3, 4, -2, -2));
   TEST_REGEX_SEARCH("\\s'+", emacs, "',# ", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("\\S'+", emacs, "',# ", match_default, make_array(3, 4, -2, -2));
   TEST_REGEX_SEARCH("\\s<+", emacs, "; ", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\S<+", emacs, "; ", match_default, make_array(1, 2, -2, -2));
   TEST_REGEX_SEARCH("\\s>+", emacs, "\n\f ", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("\\S>+", emacs, "\n\f ", match_default, make_array(2, 3, -2, -2));
}

