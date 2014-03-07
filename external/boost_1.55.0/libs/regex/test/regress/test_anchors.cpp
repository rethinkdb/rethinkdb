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

void test_anchors()
{
   // line anchors:
   using namespace boost::regex_constants;
   TEST_REGEX_SEARCH("^ab", boost::regex::extended, "ab", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("^ab", boost::regex::extended, "xxabxx", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("^ab", boost::regex::extended, "xx\nabzz", match_default, make_array(3, 5, -2, -2));
   TEST_REGEX_SEARCH("ab$", boost::regex::extended, "ab", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("ab$", boost::regex::extended, "abxx", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("ab$", boost::regex::extended, "ab\nzz", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("^ab", boost::regex::extended, "\n\n  a", match_default, make_array(-2, -2));

   TEST_REGEX_SEARCH("^ab", basic, "ab", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("^ab", basic, "xxabxx", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("^ab", basic, "xx\nabzz", match_default, make_array(3, 5, -2, -2));
   TEST_REGEX_SEARCH("ab$", basic, "ab", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("ab$", basic, "abxx", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("ab$", basic, "ab\nzz", match_default, make_array(0, 2, -2, -2));

   TEST_REGEX_SEARCH("^ab", boost::regex::extended, "ab", match_default | match_not_bol | match_not_eol, make_array(-2, -2));
   TEST_REGEX_SEARCH("^ab", boost::regex::extended, "xxabxx", match_default | match_not_bol | match_not_eol, make_array(-2, -2));
   TEST_REGEX_SEARCH("^ab", boost::regex::extended, "xx\nabzz", match_default | match_not_bol | match_not_eol, make_array(3, 5, -2, -2));
   TEST_REGEX_SEARCH("ab$", boost::regex::extended, "ab", match_default | match_not_bol | match_not_eol, make_array(-2, -2));
   TEST_REGEX_SEARCH("ab$", boost::regex::extended, "abxx", match_default | match_not_bol | match_not_eol, make_array(-2, -2));
   TEST_REGEX_SEARCH("ab$", boost::regex::extended, "ab\nzz", match_default | match_not_bol | match_not_eol, make_array(0, 2, -2, -2));

   TEST_REGEX_SEARCH("^ab", boost::regex::extended, "ab", match_default | match_single_line, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("^ab", boost::regex::extended, "xxabxx", match_default | match_single_line, make_array(-2, -2));
   TEST_REGEX_SEARCH("^ab", boost::regex::extended, "xx\nabzz", match_default | match_single_line, make_array(-2, -2));
   TEST_REGEX_SEARCH("ab$", boost::regex::extended, "ab", match_default | match_single_line, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("ab$", boost::regex::extended, "abxx", match_default | match_single_line, make_array(-2, -2));
   TEST_REGEX_SEARCH("ab$", boost::regex::extended, "ab\nzz", match_default | match_single_line, make_array(-2, -2));

   TEST_REGEX_SEARCH("^ab", boost::regex::extended, "ab", match_default | match_not_bol | match_not_eol | match_single_line, make_array(-2, -2));
   TEST_REGEX_SEARCH("^ab", boost::regex::extended, "xxabxx", match_default | match_not_bol | match_not_eol | match_single_line, make_array(-2, -2));
   TEST_REGEX_SEARCH("^ab", boost::regex::extended, "xx\nabzz", match_default | match_not_bol | match_not_eol | match_single_line, make_array(-2, -2));
   TEST_REGEX_SEARCH("ab$", boost::regex::extended, "ab", match_default | match_not_bol | match_not_eol | match_single_line, make_array(-2, -2));
   TEST_REGEX_SEARCH("ab$", boost::regex::extended, "abxx", match_default | match_not_bol | match_not_eol | match_single_line, make_array(-2, -2));
   TEST_REGEX_SEARCH("ab$", boost::regex::extended, "ab\nzz", match_default | match_not_bol | match_not_eol | match_single_line, make_array(-2, -2));
   //
   // changes to newline handling with 2.11:
   //
   TEST_REGEX_SEARCH("^.", boost::regex::extended, "  \n  \r\n  ", match_default, make_array(0, 1, -2, 3, 4, -2, 7, 8, -2, -2));
   TEST_REGEX_SEARCH(".$", boost::regex::extended, "  \n  \r\n  ", match_default, make_array(1, 2, -2, 4, 5, -2, 8, 9, -2, -2));
#if !BOOST_WORKAROUND(__BORLANDC__, < 0x560)
   TEST_REGEX_SEARCH_W(L"^.", boost::regex::extended, L"\x2028 \x2028", match_default, make_array(0, 1, -2, 1, 2, -2, -2));
   TEST_REGEX_SEARCH_W(L".$", boost::regex::extended, L" \x2028 \x2028", match_default, make_array(0, 1, -2, 2, 3, -2, 3, 4, -2, -2));
#endif
}

