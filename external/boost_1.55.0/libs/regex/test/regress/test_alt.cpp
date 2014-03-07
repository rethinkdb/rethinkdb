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

void test_alt()
{
   using namespace boost::regex_constants;
   // now test the alternation operator |
   TEST_REGEX_SEARCH("a|b", perl, "a", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("a|b", perl, "b", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("a|b|c", perl, "c", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("a|(b)|.", perl, "b", match_default, make_array(0, 1, 0, 1, -2, -2));
   TEST_REGEX_SEARCH("(a)|b|.", perl, "a", match_default, make_array(0, 1, 0, 1, -2, -2));
   TEST_REGEX_SEARCH("a(b|c)", perl, "ab", match_default, make_array(0, 2, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b|c)", perl, "ac", match_default, make_array(0, 2, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b|c)", perl, "ad", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("(a|b|c)", perl, "c", match_default, make_array(0, 1, 0, 1, -2, -2));
   TEST_REGEX_SEARCH("(a|(b)|.)", perl, "b", match_default, make_array(0, 1, 0, 1, 0, 1, -2, -2));
   TEST_INVALID_REGEX("|c", perl|no_empty_expressions);
   TEST_REGEX_SEARCH("|c", perl, " c", match_default, make_array(0, 0, -2, 1, 1, -2, 1, 2, -2, 2, 2, -2, -2));
   TEST_INVALID_REGEX("c|", perl|no_empty_expressions);
   TEST_REGEX_SEARCH("c|", perl, " c", match_default, make_array(0, 0, -2, 1, 2, -2, 2, 2, -2, -2));
   TEST_INVALID_REGEX("(|)", perl|no_empty_expressions);
   TEST_REGEX_SEARCH("(|)", perl, " c", match_default, make_array(0, 0, 0, 0, -2, 1, 1, 1, 1, -2, 2, 2, 2, 2, -2, -2));
   TEST_INVALID_REGEX("(a|)", perl|no_empty_expressions);
   TEST_REGEX_SEARCH("(a|)", perl, " a", match_default, make_array(0, 0, 0, 0, -2, 1, 2, 1, 2, -2, 2, 2, 2, 2, -2, -2));
   TEST_INVALID_REGEX("(|a)", perl|no_empty_expressions);
   TEST_REGEX_SEARCH("(|a)", perl, " a", match_default, make_array(0, 0, 0, 0, -2, 1, 1, 1, 1, -2, 1, 2, 1, 2, -2, 2, 2, 2, 2, -2, -2));
   TEST_REGEX_SEARCH("a\\|", perl, "a|", match_default, make_array(0, 2, -2, -2));

   TEST_REGEX_SEARCH("a|", basic, "a|", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("a\\|", basic, "a|", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("|", basic, "|", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("a|", basic|bk_vbar, "a|", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("a\\|b", basic|bk_vbar, "a", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("a\\|b", basic|bk_vbar, "b", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("a\nb", grep, "b", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("a\nb", grep, "a", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("a\nb", egrep, "b", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("a\nb", egrep, "a", match_default, make_array(0, 1, -2, -2));
}

