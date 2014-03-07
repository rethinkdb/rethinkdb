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

void test_forward_lookahead_asserts()
{
   //
   // forward lookahead asserts added 21/01/02
   //
   using namespace boost::regex_constants;
   TEST_REGEX_SEARCH("((?:(?!a|b)\\w)+)(\\w+)", perl, "  xxxabaxxx  ", match_default, make_array(2, 11, 2, 5, 5, 11, -2, -2));
   TEST_REGEX_SEARCH("/\\*(?:(?!\\*/).)*\\*/", perl, "  /**/  ", match_default, make_array(2, 6, -2, -2));
   TEST_REGEX_SEARCH("/\\*(?:(?!\\*/).)*\\*/", perl, "  /***/  ", match_default, make_array(2, 7, -2, -2));
   TEST_REGEX_SEARCH("/\\*(?:(?!\\*/).)*\\*/", perl, "  /********/  ", match_default, make_array(2, 12, -2, -2));
   TEST_REGEX_SEARCH("/\\*(?:(?!\\*/).)*\\*/", perl, "  /* comment */  ", match_default, make_array(2, 15, -2, -2));
   TEST_REGEX_SEARCH("<\\s*a[^>]*>((?:(?!<\\s*/\\s*a\\s*>).)*)<\\s*/\\s*a\\s*>", perl, " <a href=\"here\">here</a> ", match_default, make_array(1, 24, 16, 20, -2, -2));
   TEST_REGEX_SEARCH("<\\s*a[^>]*>((?:(?!<\\s*/\\s*a\\s*>).)*)<\\s*/\\s*a\\s*>", perl, " <a href=\"here\">here< /  a > ", match_default, make_array(1, 28, 16, 20, -2, -2));
   TEST_REGEX_SEARCH("<\\s*a[^>]*>((?:(?!<\\s*/\\s*a\\s*>).)*)(?=<\\s*/\\s*a\\s*>)", perl, " <a href=\"here\">here</a> ", match_default, make_array(1, 20, 16, 20, -2, -2));
   TEST_REGEX_SEARCH("<\\s*a[^>]*>((?:(?!<\\s*/\\s*a\\s*>).)*)(?=<\\s*/\\s*a\\s*>)", perl, " <a href=\"here\">here< /  a > ", match_default, make_array(1, 20, 16, 20, -2, -2));
   TEST_REGEX_SEARCH("^(?!^(?:PRN|AUX|CLOCK\\$|NUL|CON|COM\\d|LPT\\d|\\..*)(?:\\..+)?$)[^\\x00-\\x1f\\\\?*:\"|/]+$", perl, "command.com", match_default, make_array(0, 11, -2, -2));
   TEST_REGEX_SEARCH("^(?!^(?:PRN|AUX|CLOCK\\$|NUL|CON|COM\\d|LPT\\d|\\..*)(?:\\..+)?$)[^\\x00-\\x1f\\\\?*:\"|/]+$", perl, "PRN", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("^(?!^(?:PRN|AUX|CLOCK\\$|NUL|CON|COM\\d|LPT\\d|\\..*)(?:\\..+)?$)[^\\x00-\\x1f\\\\?*:\"|/]+$", perl, "COM2", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("^(?=.*\\d).{4,8}$", perl, "abc3", match_default, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("^(?=.*\\d).{4,8}$", perl, "abc3def4", match_default, make_array(0, 8, -2, -2));
   TEST_REGEX_SEARCH("^(?=.*\\d).{4,8}$", perl, "ab2", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("^(?=.*\\d).{4,8}$", perl, "abcdefg", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("^(?=.*\\d)(?=.*[a-z])(?=.*[A-Z]).{4,8}$", perl, "abc3", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("^(?=.*\\d)(?=.*[a-z])(?=.*[A-Z]).{4,8}$", perl, "abC3", match_default, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("^(?=.*\\d)(?=.*[a-z])(?=.*[A-Z]).{4,8}$", perl, "ABCD3", match_default, make_array(-2, -2));

   // bug report test cases:
   TEST_REGEX_SEARCH("(?=.{1,10}$).*.", perl, "AAAAA", match_default, make_array(0, 5, -2, -2));

   // lookbehind assertions, added 2004-04-30
   TEST_REGEX_SEARCH("/\\*.*(?<=\\*)/", perl, "/**/", match_default, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("/\\*.*(?<=\\*)/", perl, "/*****/   ", match_default, make_array(0, 7, -2, -2));
   TEST_REGEX_SEARCH("(?<=['\"]).*?(?=['\"])", perl, " 'ac' ", match_default, make_array(2, 4, -2, -2));
   TEST_REGEX_SEARCH("(?<=['\"]).*?(?=['\"])", perl, " \"ac\" ", match_default, make_array(2, 4, -2, -2));
   TEST_REGEX_SEARCH("(?<=['\"]).*?(?<!\\\\)(?=['\"])", perl, " \"ac\" ", match_default, make_array(2, 4, -2, -2));
   TEST_REGEX_SEARCH("(?<=['\"]).*?(?<!\\\\)(?=['\"])", perl, " \"ac\\\" \" ", match_default, make_array(2, 7, -2, -2));
   // lookbehind, with nested lookahead! :
   TEST_REGEX_SEARCH("/\\*.*(?<=(?=[[:punct:]])\\*)/", perl, "/**/", match_default, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("/\\*.*(?<=(?![[:alnum:]])\\*)/", perl, "/**/", match_default, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("/\\*.*(?<=(?>\\*))/", perl, "/**/", match_default, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("/\\*.*(?<=(?:\\*))/", perl, "/**/", match_default, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("/\\*.*(?<=(\\*))/", perl, "/**/", match_default, make_array(0, 4, 2, 3, -2, -2));
   // lookbehind with invalid content:
   TEST_INVALID_REGEX("(/)\\*.*(?<=\\1)/", perl);
   TEST_INVALID_REGEX("/\\*.*(?<=\\*+)/", perl);
   TEST_INVALID_REGEX("/\\*.*(?<=\\X)/", perl);
   TEST_INVALID_REGEX("/\\*.*(?<=[[.ae.]])/", perl);

   TEST_INVALID_REGEX("(?<=[abc]", perl);
   TEST_INVALID_REGEX("(?<=", perl);
   TEST_INVALID_REGEX("(?<", perl);
   TEST_INVALID_REGEX("(?<*", perl);
   TEST_INVALID_REGEX("(?", perl);
}

