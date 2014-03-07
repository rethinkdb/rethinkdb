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
#pragma warning(disable:4127 4428)
#endif

void test_character_escapes()
{
   using namespace boost::regex_constants;
   // characters by code
   TEST_REGEX_SEARCH("\\0101", perl, "A", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\00", perl, "\0", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\0", perl, "\0", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\0172", perl, "z", match_default, make_array(0, 1, -2, -2));
   // extra escape sequences:
   TEST_REGEX_SEARCH("\\a", perl, "\a", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\f", perl, "\f", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\n", perl, "\n", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\r", perl, "\r", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\v", perl, "\v", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\t", perl, "\t", match_default, make_array(0, 1, -2, -2));

   // updated tests for version 2:
   TEST_REGEX_SEARCH("\\x41", perl, "A", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\xff", perl, "\xff", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\xFF", perl, "\xff", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\c@", perl, "\0", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\cA", perl, "\x1", match_default, make_array(0, 1, -2, -2));
   //TEST_REGEX_SEARCH("\\cz", perl, "\x3A", match_default, make_array(0, 1, -2, -2));
   //TEST_INVALID_REGEX("\\c=", boost::regex::extended);
   //TEST_INVALID_REGEX("\\c?", boost::regex::extended);
   TEST_REGEX_SEARCH("=:", perl, "=:", match_default, make_array(0, 2, -2, -2));

   TEST_REGEX_SEARCH("\\e", perl, "\x1B", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\x1b", perl, "\x1B", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\x{1b}", perl, "\x1B", match_default, make_array(0, 1, -2, -2));
   TEST_INVALID_REGEX("\\x{}", perl);
   TEST_INVALID_REGEX("\\x{", perl);
   TEST_INVALID_REGEX("\\", perl);
   TEST_INVALID_REGEX("\\c", perl);
   TEST_INVALID_REGEX("\\x}", perl);
   TEST_INVALID_REGEX("\\x", perl);
   TEST_INVALID_REGEX("\\x{yy", perl);
   TEST_INVALID_REGEX("\\x{1b", perl);
   // \Q...\E sequences:
   TEST_INVALID_REGEX("\\Qabc\\", perl);
   TEST_REGEX_SEARCH("\\Qabc\\E", perl, "abcd", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("\\Qabc\\Ed", perl, "abcde", match_default, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("\\Q+*?\\\\E", perl, "+*?\\", match_default, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("a\\Q+*?\\\\Eb", perl, "a+*?\\b", match_default, make_array(0, 6, -2, -2));
   TEST_REGEX_SEARCH("\\C+", perl, "abcde", match_default, make_array(0, 5, -2, -2));
   TEST_REGEX_SEARCH("\\X+", perl, "abcde", match_default, make_array(0, 5, -2, -2));
#if !BOOST_WORKAROUND(__BORLANDC__, < 0x560)
   TEST_REGEX_SEARCH_W(L"\\X", perl, L"a\x0300\x0301", match_default, make_array(0, 3, -2, -2));
#endif
   // unknown escape sequences match themselves:
   TEST_REGEX_SEARCH("\\~", perl, "~", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\~", basic, "~", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\~", boost::regex::extended, "~", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\j", boost::regex::extended, "j", match_default, make_array(0, 1, -2, -2));
}

void test_assertion_escapes()
{
   using namespace boost::regex_constants;
   // word start:
   TEST_REGEX_SEARCH("\\<abcd", perl, "  abcd", match_default, make_array(2, 6, -2, -2));
   TEST_REGEX_SEARCH("\\<ab", perl, "cab", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("\\<ab", perl, "\nab", match_default, make_array(1, 3, -2, -2));
   TEST_REGEX_SEARCH("\\<tag", perl, "::tag", match_default, make_array(2, 5, -2, -2));
   TEST_REGEX_SEARCH("\\<abcd", perl, "abcd", match_default|match_not_bow, make_array(-2, -2));
   TEST_REGEX_SEARCH("\\<abcd", perl, "  abcd", match_default|match_not_bow, make_array(2, 6, -2, -2));
   TEST_REGEX_SEARCH("\\<", perl, "ab ", match_default|match_not_bow, make_array(-2, -2));
   TEST_REGEX_SEARCH(".\\<.", perl, "ab", match_default|match_not_bow, make_array(-2, -2));
   TEST_REGEX_SEARCH(".\\<.", perl, " b", match_default|match_not_bow, make_array(0, 2, -2, -2));
   // word end:
   TEST_REGEX_SEARCH("abc\\>", perl, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("abc\\>", perl, "abcd", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("abc\\>", perl, "abc\n", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("abc\\>", perl, "abc::", match_default, make_array(0,3, -2, -2));
   TEST_REGEX_SEARCH("abc(?:\\>..|$)", perl, "abc::", match_default, make_array(0, 5, -2, -2));
   TEST_REGEX_SEARCH("\\>", perl, "  ", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH(".\\>.", perl, "  ", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("abc\\>", perl, "abc", match_default|match_not_eow, make_array(-2, -2));
   // word boundary:
   TEST_REGEX_SEARCH("\\babcd", perl, "  abcd", match_default, make_array(2, 6, -2, -2));
   TEST_REGEX_SEARCH("\\bab", perl, "cab", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("\\bab", perl, "\nab", match_default, make_array(1, 3, -2, -2));
   TEST_REGEX_SEARCH("\\btag", perl, "::tag", match_default, make_array(2, 5, -2, -2));
   TEST_REGEX_SEARCH("abc\\b", perl, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("abc\\b", perl, "abcd", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("abc\\b", perl, "abc\n", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("abc\\b", perl, "abc::", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("\\babcd", perl, "abcd", match_default|match_not_bow, make_array(-2, -2));
   // within word:
   TEST_REGEX_SEARCH("\\B", perl, "ab", match_default, make_array(1, 1, -2, -2));
   TEST_REGEX_SEARCH("a\\Bb", perl, "ab", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("a\\B", perl, "ab", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("a\\B", perl, "a", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("a\\B", perl, "a ", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("\\By\\b", perl, "xy", match_default, make_array(1, 2, -2, -2));
   TEST_REGEX_SEARCH("\\by\\B", perl, "yz", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\B\\*\\B", perl, " * ", match_default, make_array(1, 2, -2, -2));
   // buffer operators:
   TEST_REGEX_SEARCH("\\`abc", perl, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("\\`abc", perl, "\nabc", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("\\`abc", perl, " abc", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("abc\\'", perl, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("abc\\'", perl, "abc\n", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("abc\\'", perl, "abc ", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("abc(?:\\'|$)", perl, "abc", match_default, make_array(0, 3, -2, -2));

   // word start:
   TEST_REGEX_SEARCH("[[:<:]]abcd", perl, "  abcd", match_default, make_array(2, 6, -2, -2));
   TEST_REGEX_SEARCH("[[:<:]]ab", perl, "cab", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("[[:<:]]ab", perl, "\nab", match_default, make_array(1, 3, -2, -2));
   TEST_REGEX_SEARCH("[[:<:]]tag", perl, "::tag", match_default, make_array(2, 5, -2, -2));
   // word end
   TEST_REGEX_SEARCH("abc[[:>:]]", perl, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("abc[[:>:]]", perl, "abcd", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("abc[[:>:]]", perl, "abc\n", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("abc[[:>:]]", perl, "abc::", match_default, make_array(0, 3, -2, -2));

   TEST_REGEX_SEARCH("\\Aabc", perl, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("\\Aabc", perl, "aabc", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("abc\\z", perl, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("abc\\z", perl, "abcd", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("abc\\Z", perl, "abc\n\n", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("abc\\Z", perl, "abc\n\n", match_default|match_not_eob, make_array(-2, -2));
   TEST_REGEX_SEARCH("abc\\Z", perl, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("\\Gabc", perl, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("\\Gabc", perl, "dabcd", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("a\\Gbc", perl, "abc", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("a\\Aab", perl, "abc", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("abc(?:\\Z|$)", perl, "abc\n\n", match_default, make_array(0, 3, -2, -2));

   // Buffer reset \K:
   TEST_REGEX_SEARCH("(foo)\\Kbar", perl, "foobar", match_default, make_array(3, 6, 0, 3, -2, -2));
   TEST_REGEX_SEARCH("(foo)(\\Kbar|baz)", perl, "foobar", match_default, make_array(3, 6, 0, 3, 3, 6, -2, -2));
   TEST_REGEX_SEARCH("(foo)(\\Kbar|baz)", perl, "foobaz", match_default, make_array(0, 6, 0, 3, 3, 6, -2, -2));
   TEST_REGEX_SEARCH("(foo\\Kbar)baz", perl, "foobarbaz", match_default, make_array(3, 9, 0, 6, -2, -2));

   // Line ending \R:
   TEST_REGEX_SEARCH("\\R", perl, "foo\nbar", match_default, make_array(3, 4, -2, -2));
   TEST_REGEX_SEARCH("\\R", perl, "foo\rbar", match_default, make_array(3, 4, -2, -2));
   TEST_REGEX_SEARCH("\\R", perl, "foo\r\nbar", match_default, make_array(3, 5, -2, -2));
   // see if \u works:
   const wchar_t* w = L"\u2028";
   if(*w == 0x2028u)
   {
      TEST_REGEX_SEARCH_W(L"\\R", perl, L"foo\u2028bar", match_default, make_array(3, 4, -2, -2));
      TEST_REGEX_SEARCH_W(L"\\R", perl, L"foo\u2029bar", match_default, make_array(3, 4, -2, -2));
   }
}

