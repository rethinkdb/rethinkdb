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
  *   FILE         basic_tests.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: main regex test declarations.
  */

#include <boost/detail/workaround.hpp>

#if BOOST_WORKAROUND(__BORLANDC__, < 0x560)
// we get unresolved externals from basic_string
// unless we do this, a well known Borland bug:
#define _RWSTD_COMPILE_INSTANTIATE
#endif

#include "test.hpp"

#ifdef BOOST_MSVC
#pragma warning(disable:4127)
#endif

void basic_tests()
{
   using namespace boost::regex_constants;
   TEST_REGEX_SEARCH("a", basic, "a", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("a", basic, "bba", match_default, make_array(2, 3, -2, -2));
   TEST_REGEX_SEARCH("Z", perl, "aaa", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("Z", perl, "xxxxZZxxx", match_default, make_array(4, 5, -2, 5, 6, -2, -2));
   // and some simple brackets:
   TEST_REGEX_SEARCH("(a)", perl, "zzzaazz", match_default, make_array(3, 4, 3, 4, -2, 4, 5, 4, 5, -2, -2));
   TEST_REGEX_SEARCH("()", perl, "zzz", match_default, make_array(0, 0, 0, 0, -2, 1, 1, 1, 1, -2, 2, 2, 2, 2, -2, 3, 3, 3, 3, -2, -2));
   TEST_REGEX_SEARCH("()", perl, "", match_default, make_array(0, 0, 0, 0, -2, -2));
   TEST_INVALID_REGEX("(", perl);
   TEST_INVALID_REGEX("", perl|no_empty_expressions);
   TEST_REGEX_SEARCH("", perl, "abc", match_default, make_array(0, 0, -2, 1, 1, -2, 2, 2, -2, 3, 3, -2, -2));
   TEST_INVALID_REGEX(")", perl);
   TEST_INVALID_REGEX("(aa", perl);
   TEST_INVALID_REGEX("aa)", perl);
   TEST_REGEX_SEARCH("a", perl, "b", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("\\(\\)", perl, "()", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("\\(a\\)", perl, "(a)", match_default, make_array(0, 3, -2, -2));
   TEST_INVALID_REGEX("\\()", perl);
   TEST_INVALID_REGEX("(\\)", perl);
   TEST_REGEX_SEARCH("p(a)rameter", perl, "ABCparameterXYZ", match_default, make_array(3, 12, 4, 5, -2, -2));
   TEST_REGEX_SEARCH("[pq](a)rameter", perl, "ABCparameterXYZ", match_default, make_array(3, 12, 4, 5, -2, -2));

   // now try escaped brackets:
   TEST_REGEX_SEARCH("\\(a\\)", basic, "zzzaazz", match_default, make_array(3, 4, 3, 4, -2, 4, 5, 4, 5, -2, -2));
   TEST_REGEX_SEARCH("\\(\\)", basic, "zzz", match_default, make_array(0, 0, 0, 0, -2, 1, 1, 1, 1, -2, 2, 2, 2, 2, -2, 3, 3, 3, 3, -2, -2));
   TEST_REGEX_SEARCH("\\(\\)", basic, "", match_default, make_array(0, 0, 0, 0, -2, -2));
   TEST_INVALID_REGEX("\\(", basic);
   TEST_INVALID_REGEX("\\)", basic);
   TEST_INVALID_REGEX("\\(aa", basic);
   TEST_INVALID_REGEX("aa\\)", basic);
   TEST_REGEX_SEARCH("()", basic, "()", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("(a)", basic, "(a)", match_default, make_array(0, 3, -2, -2));
   TEST_INVALID_REGEX("\\()", basic);
   TEST_INVALID_REGEX("(\\)", basic);
   TEST_REGEX_SEARCH("p\\(a\\)rameter", basic, "ABCparameterXYZ", match_default, make_array(3, 12, 4, 5, -2, -2));
   TEST_REGEX_SEARCH("[pq]\\(a\\)rameter", basic, "ABCparameterXYZ", match_default, make_array(3, 12, 4, 5, -2, -2));

   // now move on to "." wildcards
   TEST_REGEX_SEARCH(".", perl, "a", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH(".", perl, "\n", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH(".", perl, "\r", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH(".", perl, "\0", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH(".", perl, "a", match_not_dot_newline, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH(".", perl, "\n", match_not_dot_newline, make_array(-2, -2));
   TEST_REGEX_SEARCH(".", perl, "\r", match_not_dot_newline, make_array(-2, -2));
   TEST_REGEX_SEARCH(".", perl, "\0", match_not_dot_newline, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH(".", perl, "\n", match_not_dot_null | match_not_dot_newline, make_array(-2, -2));
   TEST_REGEX_SEARCH(".", perl, "\r", match_not_dot_null | match_not_dot_newline, make_array(-2, -2));
   TEST_REGEX_SEARCH(".", perl, "\0", match_not_dot_null | match_not_dot_newline, make_array(-2, -2));
   TEST_REGEX_SEARCH(".", basic, "a", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH(".", basic, "\n", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH(".", basic, "\r", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH(".", basic, "\0", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH(".", basic, "a", match_not_dot_newline, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH(".", basic, "\n", match_not_dot_newline, make_array(-2, -2));
   TEST_REGEX_SEARCH(".", basic, "\r", match_not_dot_newline, make_array(-2, -2));
   TEST_REGEX_SEARCH(".", basic, "\0", match_not_dot_newline, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH(".", basic, "\n", match_not_dot_null | match_not_dot_newline, make_array(-2, -2));
   TEST_REGEX_SEARCH(".", basic, "\r", match_not_dot_null | match_not_dot_newline, make_array(-2, -2));
   TEST_REGEX_SEARCH(".", basic, "\0", match_not_dot_null | match_not_dot_newline, make_array(-2, -2));
}

void test_non_marking_paren()
{
   using namespace boost::regex_constants;
   //
   // non-marking parenthesis added 25/04/00
   //
   TEST_REGEX_SEARCH("(?:abc)+", perl, "xxabcabcxx", match_default, make_array(2, 8, -2, -2));
   TEST_REGEX_SEARCH("(?:a+)(b+)", perl, "xaaabbbx", match_default, make_array(1, 7, 4, 7, -2, -2));
   TEST_REGEX_SEARCH("(a+)(?:b+)", perl, "xaaabbba", match_default, make_array(1, 7, 1, 4, -2, -2));
   TEST_REGEX_SEARCH("(?:(a+)b+)", perl, "xaaabbba", match_default, make_array(1, 7, 1, 4, -2, -2));
   TEST_REGEX_SEARCH("(?:a+(b+))", perl, "xaaabbba", match_default, make_array(1, 7, 4, 7, -2, -2));
   TEST_REGEX_SEARCH("a+(?#b+)b+", perl, "xaaabbba", match_default, make_array(1, 7, -2, -2));
   TEST_REGEX_SEARCH("(a)(?:b|$)", perl, "ab", match_default, make_array(0, 2, 0, 1, -2, -2));
   TEST_REGEX_SEARCH("(a)(?:b|$)", perl, "a", match_default, make_array(0, 1, 0, 1, -2, -2));
}

void test_partial_match()
{
   using namespace boost::regex_constants;
   //
   // try some partial matches:
   //
   TEST_REGEX_SEARCH("(xyz)(.*)abc", perl, "xyzaaab", match_default|match_partial, make_array(0, 7, -2, -2));
   TEST_REGEX_SEARCH("(xyz)(.*)abc", perl, "xyz", match_default|match_partial, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("(xyz)(.*)abc", perl, "xy", match_default|match_partial, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("(xyz)(.*)abc", perl, "x", match_default|match_partial, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("(xyz)(.*)abc", perl, "", match_default|match_partial, make_array(-2, -2));
   TEST_REGEX_SEARCH("(xyz)(.*)abc", perl, "aaaa", match_default|match_partial, make_array(-2, -2));
   TEST_REGEX_SEARCH(".abc", perl, "aaab", match_default|match_partial, make_array(1, 4, -2, -2));
   TEST_REGEX_SEARCH("a[_]", perl, "xxa", match_default|match_partial, make_array(2, 3, -2, -2));
   TEST_REGEX_SEARCH(".{4,}", perl, "xxa", match_default|match_partial, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH(".{4,}", perl, "xxa", match_default|match_partial|match_not_dot_null, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("[\\x0-\\xff]{4,}", perl, "xxa", match_default|match_partial, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a{4,}", perl, "aaa", match_default|match_partial, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("\\w{4,}", perl, "aaa", match_default|match_partial, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH(".*?<tag>", perl, "aaa", match_default|match_partial, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a*?<tag>", perl, "aaa", match_default|match_partial, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("\\w*?<tag>", perl, "aaa", match_default|match_partial, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("(\\w)*?<tag>", perl, "aaa", match_default|match_partial, make_array(0, 3, -2, -2));

   TEST_REGEX_SEARCH("(xyz)(.*)abc", extended, "xyzaaab", match_default|match_partial, make_array(0, 7, -2, -2));
   TEST_REGEX_SEARCH("(xyz)(.*)abc", extended, "xyz", match_default|match_partial, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("(xyz)(.*)abc", extended, "xy", match_default|match_partial, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("(xyz)(.*)abc", extended, "x", match_default|match_partial, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("(xyz)(.*)abc", extended, "", match_default|match_partial, make_array(-2, -2));
   TEST_REGEX_SEARCH("(xyz)(.*)abc", extended, "aaaa", match_default|match_partial, make_array(-2, -2));
   TEST_REGEX_SEARCH(".abc", extended, "aaab", match_default|match_partial, make_array(1, 4, -2, -2));
   TEST_REGEX_SEARCH("a[_]", extended, "xxa", match_default|match_partial, make_array(2, 3, -2, -2));
   TEST_REGEX_SEARCH(".{4,}", extended, "xxa", match_default|match_partial, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH(".{4,}", extended, "xxa", match_default|match_partial|match_not_dot_null, make_array(0, 3, -2, -2));
}

void test_nosubs()
{
   using namespace boost::regex_constants;
   // subtleties of matching with no sub-expressions marked
   TEST_REGEX_SEARCH("a(b?c)+d", perl, "accd", match_default|match_nosubs, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("(wee|week)(knights|night)", perl, "weeknights", match_default|match_nosubs, make_array(0, 10, -2, -2));
   TEST_REGEX_SEARCH(".*", perl, "abc", match_default|match_nosubs, make_array(0, 3, -2, 3, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|(c))d", perl, "abd", match_default|match_nosubs, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|(c))d", perl, "acd", match_default|match_nosubs, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b*|c|e)d", perl, "abbd", match_default|match_nosubs, make_array(0, 4, -2, -2));
   
   TEST_REGEX_SEARCH("a(b*|c|e)d", perl, "acd", match_default|match_nosubs, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b*|c|e)d", perl, "ad", match_default|match_nosubs, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b?)c", perl, "abc", match_default|match_nosubs, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b?)c", perl, "ac", match_default|match_nosubs, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b+)c", perl, "abc", match_default|match_nosubs, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b+)c", perl, "abbbc", match_default|match_nosubs, make_array(0, 5, -2, -2));
   TEST_REGEX_SEARCH("a(b*)c", perl, "ac", match_default|match_nosubs, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("(a|ab)(bc([de]+)f|cde)", perl, "abcdef", match_default|match_nosubs, make_array(0, 6, -2, -2));
   TEST_REGEX_SEARCH("a([bc]?)c", perl, "abc", match_default|match_nosubs, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a([bc]?)c", perl, "ac", match_default|match_nosubs, make_array(0, 2, -2, -2));

   TEST_REGEX_SEARCH("a([bc]+)c", perl, "abc", match_default|match_nosubs, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a([bc]+)c", perl, "abcc", match_default|match_nosubs, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("a([bc]+)bc", perl, "abcbc", match_default|match_nosubs, make_array(0, 5, -2, -2));
   TEST_REGEX_SEARCH("a(bb+|b)b", perl, "abb", match_default|match_nosubs, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a(bbb+|bb+|b)b", perl, "abb", match_default|match_nosubs, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a(bbb+|bb+|b)b", perl, "abbb", match_default|match_nosubs, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("a(bbb+|bb+|b)bb", perl, "abbb", match_default|match_nosubs, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("(.*).*", perl, "abcdef", match_default|match_nosubs, make_array(0, 6, -2, 6, 6, -2, -2));
   TEST_REGEX_SEARCH("(a*)*", perl, "bc", match_default|match_nosubs, make_array(0, 0, -2, 1, 1, -2, 2, 2, -2, -2));

   TEST_REGEX_SEARCH("a(b?c)+d", perl|nosubs, "accd", match_default, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("(wee|week)(knights|night)", perl|nosubs, "weeknights", match_default, make_array(0, 10, -2, -2));
   TEST_REGEX_SEARCH(".*", perl|nosubs, "abc", match_default, make_array(0, 3, -2, 3, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|(c))d", perl|nosubs, "abd", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|(c))d", perl|nosubs, "acd", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b*|c|e)d", perl|nosubs, "abbd", match_default, make_array(0, 4, -2, -2));
   
   TEST_REGEX_SEARCH("a(b*|c|e)d", perl|nosubs, "acd", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b*|c|e)d", perl|nosubs, "ad", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b?)c", perl|nosubs, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b?)c", perl|nosubs, "ac", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b+)c", perl|nosubs, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b+)c", perl|nosubs, "abbbc", match_default, make_array(0, 5, -2, -2));
   TEST_REGEX_SEARCH("a(b*)c", perl|nosubs, "ac", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("(a|ab)(bc([de]+)f|cde)", perl|nosubs, "abcdef", match_default, make_array(0, 6, -2, -2));
   TEST_REGEX_SEARCH("a([bc]?)c", perl|nosubs, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a([bc]?)c", perl|nosubs, "ac", match_default, make_array(0, 2, -2, -2));

   TEST_REGEX_SEARCH("a([bc]+)c", perl|nosubs, "abc", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a([bc]+)c", perl|nosubs, "abcc", match_default, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("a([bc]+)bc", perl|nosubs, "abcbc", match_default, make_array(0, 5, -2, -2));
   TEST_REGEX_SEARCH("a(bb+|b)b", perl|nosubs, "abb", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a(bbb+|bb+|b)b", perl|nosubs, "abb", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a(bbb+|bb+|b)b", perl|nosubs, "abbb", match_default, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("a(bbb+|bb+|b)bb", perl|nosubs, "abbb", match_default, make_array(0, 4, -2, -2));
   TEST_REGEX_SEARCH("(.*).*", perl|nosubs, "abcdef", match_default, make_array(0, 6, -2, 6, 6, -2, -2));
   TEST_REGEX_SEARCH("(a*)*", perl|nosubs, "bc", match_default, make_array(0, 0, -2, 1, 1, -2, 2, 2, -2, -2));
}


