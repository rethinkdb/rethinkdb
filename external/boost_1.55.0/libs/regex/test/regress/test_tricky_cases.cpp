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

void test_tricky_cases2();
void test_tricky_cases3();

void test_tricky_cases()
{
   using namespace boost::regex_constants;
   //
   // now follows various complex expressions designed to try and bust the matcher:
   //
   TEST_REGEX_SEARCH("a(((b)))c", perl, "abc", match_default, make_array(0, 3, 1, 2, 1, 2, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b|(c))d", perl, "abd", match_default, make_array(0, 3, 1, 2, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("a(b|(c))d", perl, "acd", match_default, make_array(0, 3, 1, 2, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b*|c)d", perl, "abbd", match_default, make_array(0, 4, 1, 3, -2, -2));
   // just gotta have one DFA-buster, of course
   TEST_REGEX_SEARCH("a[ab]{20}", perl, "aaaaabaaaabaaaabaaaab", match_default, make_array(0, 21, -2, -2));
   // and an inline expansion in case somebody gets tricky
   TEST_REGEX_SEARCH("a[ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab]", perl, "aaaaabaaaabaaaabaaaab", match_default, make_array(0, 21, -2, -2));
   // and in case somebody just slips in an NFA...
   TEST_REGEX_SEARCH("a[ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab](wee|week)(knights|night)", perl, "aaaaabaaaabaaaabaaaabweeknights", match_default, make_array(0, 31, 21, 24, 24, 31, -2, -2));
   // one really big one
   TEST_REGEX_SEARCH("1234567890123456789012345678901234567890123456789012345678901234567890", perl, "a1234567890123456789012345678901234567890123456789012345678901234567890b", match_default, make_array(1, 71, -2, -2));
   // fish for problems as brackets go past 8
   TEST_REGEX_SEARCH("[ab][cd][ef][gh][ij][kl][mn]", perl, "xacegikmoq", match_default, make_array(1, 8, -2, -2));
   TEST_REGEX_SEARCH("[ab][cd][ef][gh][ij][kl][mn][op]", perl, "xacegikmoq", match_default, make_array(1, 9, -2, -2));
   TEST_REGEX_SEARCH("[ab][cd][ef][gh][ij][kl][mn][op][qr]", perl, "xacegikmoqy", match_default, make_array(1, 10, -2, -2));
   TEST_REGEX_SEARCH("[ab][cd][ef][gh][ij][kl][mn][op][q]", perl, "xacegikmoqy", match_default, make_array(1, 10, -2, -2));
   // and as parenthesis go past 9:
   TEST_REGEX_SEARCH("(a)(b)(c)(d)(e)(f)(g)(h)", perl, "zabcdefghi", match_default, make_array(1, 9, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, -2, -2));
   TEST_REGEX_SEARCH("(a)(b)(c)(d)(e)(f)(g)(h)(i)", perl, "zabcdefghij", match_default, make_array(1, 10, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, -2, -2));
   TEST_REGEX_SEARCH("(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)", perl, "zabcdefghijk", match_default, make_array(1, 11, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, -2, -2));
   TEST_REGEX_SEARCH("(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)(k)", perl, "zabcdefghijkl", match_default, make_array(1, 12, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, -2, -2));
   TEST_REGEX_SEARCH("(a)d|(b)c", perl, "abc", match_default, make_array(1, 3, -1, -1, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("_+((www)|(ftp)|(mailto)):_*", perl, "_wwwnocolon _mailto:", match_default, make_array(12, 20, 13, 19, -1, -1, -1, -1, 13, 19, -2, -2));
   // subtleties of matching
   TEST_REGEX_SEARCH("a(b)?c\\1d", perl, "acd", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("a(b?c)+d", perl, "accd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("(wee|week)(knights|night)", perl, "weeknights", match_default, make_array(0, 10, 0, 3, 3, 10, -2, -2));
   TEST_REGEX_SEARCH(".*", perl, "abc", match_default, make_array(0, 3, -2, 3, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|(c))d", perl, "abd", match_default, make_array(0, 3, 1, 2, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("a(b|(c))d", perl, "acd", match_default, make_array(0, 3, 1, 2, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b*|c|e)d", perl, "abbd", match_default, make_array(0, 4, 1, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b*|c|e)d", perl, "acd", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b*|c|e)d", perl, "ad", match_default, make_array(0, 2, 1, 1, -2, -2));
   TEST_REGEX_SEARCH("a(b?)c", perl, "abc", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b?)c", perl, "ac", match_default, make_array(0, 2, 1, 1, -2, -2));
   TEST_REGEX_SEARCH("a(b+)c", perl, "abc", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b+)c", perl, "abbbc", match_default, make_array(0, 5, 1, 4, -2, -2));
   TEST_REGEX_SEARCH("a(b*)c", perl, "ac", match_default, make_array(0, 2, 1, 1, -2, -2));
   TEST_REGEX_SEARCH("(a|ab)(bc([de]+)f|cde)", perl, "abcdef", match_default, make_array(0, 6, 0, 1, 1, 6, 3, 5, -2, -2));
   TEST_REGEX_SEARCH("a([bc]?)c", perl, "abc", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a([bc]?)c", perl, "ac", match_default, make_array(0, 2, 1, 1, -2, -2));
   TEST_REGEX_SEARCH("a([bc]+)c", perl, "abc", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a([bc]+)c", perl, "abcc", match_default, make_array(0, 4, 1, 3, -2, -2));
   TEST_REGEX_SEARCH("a([bc]+)bc", perl, "abcbc", match_default, make_array(0, 5, 1, 3, -2, -2));
   TEST_REGEX_SEARCH("a(bb+|b)b", perl, "abb", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(bbb+|bb+|b)b", perl, "abb", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(bbb+|bb+|b)b", perl, "abbb", match_default, make_array(0, 4, 1, 3, -2, -2));
   TEST_REGEX_SEARCH("a(bbb+|bb+|b)bb", perl, "abbb", match_default, make_array(0, 4, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("(.*).*", perl, "abcdef", match_default, make_array(0, 6, 0, 6, -2, 6, 6, 6, 6, -2, -2));
   TEST_REGEX_SEARCH("(a*)*", perl, "bc", match_default, make_array(0, 0, 0, 0, -2, 1, 1, 1, 1, -2, 2, 2, 2, 2, -2, -2));
   TEST_REGEX_SEARCH("Z(((((((a+)+)+)+)+)+)+)+|Y(((((((a+)+)+)+)+)+)+)+|X(((((((a+)+)+)+)+)+)+)+|W(((((((a+)+)+)+)+)+)+)+|V(((((((a+)+)+)+)+)+)+)+|CZ(((((((a+)+)+)+)+)+)+)+|CY(((((((a+)+)+)+)+)+)+)+|CX(((((((a+)+)+)+)+)+)+)+|CW(((((((a+)+)+)+)+)+)+)+|CV(((((((a+)+)+)+)+)+)+)+|(a+)+", perl, "bc", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("Z(((((((a+)+)+)+)+)+)+)+|Y(((((((a+)+)+)+)+)+)+)+|X(((((((a+)+)+)+)+)+)+)+|W(((((((a+)+)+)+)+)+)+)+|V(((((((a+)+)+)+)+)+)+)+|CZ(((((((a+)+)+)+)+)+)+)+|CY(((((((a+)+)+)+)+)+)+)+|CX(((((((a+)+)+)+)+)+)+)+|CW(((((((a+)+)+)+)+)+)+)+|CV(((((((a+)+)+)+)+)+)+)+|(a+)+", perl, "aaa", match_default, 
      make_array(0, 3, 
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
      0, 3,
      -2, -2));
   TEST_REGEX_SEARCH("Z(((((((a+)+)+)+)+)+)+)+|Y(((((((a+)+)+)+)+)+)+)+|X(((((((a+)+)+)+)+)+)+)+|W(((((((a+)+)+)+)+)+)+)+|V(((((((a+)+)+)+)+)+)+)+|CZ(((((((a+)+)+)+)+)+)+)+|CY(((((((a+)+)+)+)+)+)+)+|CX(((((((a+)+)+)+)+)+)+)+|CW(((((((a+)+)+)+)+)+)+)+|CV(((((((a+)+)+)+)+)+)+)+|(a+)+", 
      perl, "Zaaa", match_default, 
      make_array(0, 4, 
      1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, -1, -1, -1, -1, -1, -1, 
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
      -1, -1,
      -2, -2));
   TEST_REGEX_SEARCH("xyx*xz", perl, "xyxxxxyxxxz", match_default, make_array(5, 11, -2, -2));
   // do we get the right subexpression when it is used more than once?
   TEST_REGEX_SEARCH("a(b|c)*d", perl, "ad", match_default, make_array(0, 2, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("a(b|c)*d", perl, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c)+d", perl, "abd", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b|c)+d", perl, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c?)+d", perl, "ad", match_default, make_array(0, 2, 1, 1, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){0,0}d", perl, "ad", match_default, make_array(0, 2, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){0,1}d", perl, "ad", match_default, make_array(0, 2, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){0,1}d", perl, "abd", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){0,2}d", perl, "ad", match_default, make_array(0, 2, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){0,2}d", perl, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){0,}d", perl, "ad", match_default, make_array(0, 2, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){0,}d", perl, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){1,1}d", perl, "abd", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){1,2}d", perl, "abd", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){1,2}d", perl, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){1,}d", perl, "abd", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){1,}d", perl, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){2,2}d", perl, "acbd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){2,2}d", perl, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){2,4}d", perl, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){2,4}d", perl, "abcbd", match_default, make_array(0, 5, 3, 4, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){2,4}d", perl, "abcbcd", match_default, make_array(0, 6, 4, 5, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){2,}d", perl, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){2,}d", perl, "abcbd", match_default, make_array(0, 5, 3, 4, -2, -2));

   test_tricky_cases2();
   test_tricky_cases3();
}

void test_tricky_cases2()
{
   using namespace boost::regex_constants;
   
   TEST_REGEX_SEARCH("a(((b)))c", boost::regex::extended, "abc", match_default, make_array(0, 3, 1, 2, 1, 2, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b|(c))d", boost::regex::extended, "abd", match_default, make_array(0, 3, 1, 2, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("a(b|(c))d", boost::regex::extended, "acd", match_default, make_array(0, 3, 1, 2, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b*|c)d", boost::regex::extended, "abbd", match_default, make_array(0, 4, 1, 3, -2, -2));
   // just gotta have one DFA-buster, of course
   TEST_REGEX_SEARCH("a[ab]{20}", boost::regex::extended, "aaaaabaaaabaaaabaaaab", match_default, make_array(0, 21, -2, -2));
   // and an inline expansion in case somebody gets tricky
   TEST_REGEX_SEARCH("a[ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab]", boost::regex::extended, "aaaaabaaaabaaaabaaaab", match_default, make_array(0, 21, -2, -2));
   // and in case somebody just slips in an NFA...
   TEST_REGEX_SEARCH("a[ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab][ab](wee|week)(knights|night)", boost::regex::extended, "aaaaabaaaabaaaabaaaabweeknights", match_default, make_array(0, 31, 21, 24, 24, 31, -2, -2));
   // one really big one
   TEST_REGEX_SEARCH("1234567890123456789012345678901234567890123456789012345678901234567890", boost::regex::extended, "a1234567890123456789012345678901234567890123456789012345678901234567890b", match_default, make_array(1, 71, -2, -2));
   // fish for problems as brackets go past 8
   TEST_REGEX_SEARCH("[ab][cd][ef][gh][ij][kl][mn]", boost::regex::extended, "xacegikmoq", match_default, make_array(1, 8, -2, -2));
   TEST_REGEX_SEARCH("[ab][cd][ef][gh][ij][kl][mn][op]", boost::regex::extended, "xacegikmoq", match_default, make_array(1, 9, -2, -2));
   TEST_REGEX_SEARCH("[ab][cd][ef][gh][ij][kl][mn][op][qr]", boost::regex::extended, "xacegikmoqy", match_default, make_array(1, 10, -2, -2));
   TEST_REGEX_SEARCH("[ab][cd][ef][gh][ij][kl][mn][op][q]", boost::regex::extended, "xacegikmoqy", match_default, make_array(1, 10, -2, -2));
   // and as parenthesis go past 9:
   TEST_REGEX_SEARCH("(a)(b)(c)(d)(e)(f)(g)(h)", boost::regex::extended, "zabcdefghi", match_default, make_array(1, 9, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, -2, -2));
   TEST_REGEX_SEARCH("(a)(b)(c)(d)(e)(f)(g)(h)(i)", boost::regex::extended, "zabcdefghij", match_default, make_array(1, 10, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, -2, -2));
   TEST_REGEX_SEARCH("(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)", boost::regex::extended, "zabcdefghijk", match_default, make_array(1, 11, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, -2, -2));
   TEST_REGEX_SEARCH("(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)(k)", boost::regex::extended, "zabcdefghijkl", match_default, make_array(1, 12, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, -2, -2));
   TEST_REGEX_SEARCH("(a)d|(b)c", boost::regex::extended, "abc", match_default, make_array(1, 3, -1, -1, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("_+((www)|(ftp)|(mailto)):_*", boost::regex::extended, "_wwwnocolon _mailto:", match_default, make_array(12, 20, 13, 19, -1, -1, -1, -1, 13, 19, -2, -2));
   // subtleties of matching
   TEST_REGEX_SEARCH("a\\(b\\)\\?c\\1d", basic|bk_plus_qm, "acd", match_default, make_array(0, 3, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("a(b?c)+d", boost::regex::extended, "accd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("(wee|week)(knights|night)", boost::regex::extended, "weeknights", match_default, make_array(0, 10, 0, 3, 3, 10, -2, -2));
   TEST_REGEX_SEARCH(".*", boost::regex::extended, "abc", match_default, make_array(0, 3, -2, 3, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|(c))d", boost::regex::extended, "abd", match_default, make_array(0, 3, 1, 2, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("a(b|(c))d", boost::regex::extended, "acd", match_default, make_array(0, 3, 1, 2, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b*|c|e)d", boost::regex::extended, "abbd", match_default, make_array(0, 4, 1, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b*|c|e)d", boost::regex::extended, "acd", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b*|c|e)d", boost::regex::extended, "ad", match_default, make_array(0, 2, 1, 1, -2, -2));
   TEST_REGEX_SEARCH("a(b?)c", boost::regex::extended, "abc", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b?)c", boost::regex::extended, "ac", match_default, make_array(0, 2, 1, 1, -2, -2));
   TEST_REGEX_SEARCH("a(b+)c", boost::regex::extended, "abc", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b+)c", boost::regex::extended, "abbbc", match_default, make_array(0, 5, 1, 4, -2, -2));
   TEST_REGEX_SEARCH("a(b*)c", boost::regex::extended, "ac", match_default, make_array(0, 2, 1, 1, -2, -2));
   TEST_REGEX_SEARCH("(a|ab)(bc([de]+)f|cde)", boost::regex::extended, "abcdef", match_default, make_array(0, 6, 0, 1, 1, 6, 3, 5, -2, -2));
   TEST_REGEX_SEARCH("a([bc]?)c", boost::regex::extended, "abc", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a([bc]?)c", boost::regex::extended, "ac", match_default, make_array(0, 2, 1, 1, -2, -2));
   TEST_REGEX_SEARCH("a([bc]+)c", boost::regex::extended, "abc", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a([bc]+)c", boost::regex::extended, "abcc", match_default, make_array(0, 4, 1, 3, -2, -2));
   TEST_REGEX_SEARCH("a([bc]+)bc", boost::regex::extended, "abcbc", match_default, make_array(0, 5, 1, 3, -2, -2));
   TEST_REGEX_SEARCH("a(bb+|b)b", boost::regex::extended, "abb", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(bbb+|bb+|b)b", boost::regex::extended, "abb", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(bbb+|bb+|b)b", boost::regex::extended, "abbb", match_default, make_array(0, 4, 1, 3, -2, -2));
   TEST_REGEX_SEARCH("a(bbb+|bb+|b)bb", boost::regex::extended, "abbb", match_default, make_array(0, 4, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("(.*).*", boost::regex::extended, "abcdef", match_default, make_array(0, 6, 0, 6, -2, 6, 6, 6, 6, -2, -2));
   TEST_REGEX_SEARCH("(a*)*", boost::regex::extended, "bc", match_default, make_array(0, 0, 0, 0, -2, 1, 1, 1, 1, -2, 2, 2, 2, 2, -2, -2));
   TEST_REGEX_SEARCH("xyx*xz", boost::regex::extended, "xyxxxxyxxxz", match_default, make_array(5, 11, -2, -2));
   // do we get the right subexpression when it is used more than once?
   TEST_REGEX_SEARCH("a(b|c)*d", boost::regex::extended, "ad", match_default, make_array(0, 2, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("a(b|c)*d", boost::regex::extended, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c)+d", boost::regex::extended, "abd", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b|c)+d", boost::regex::extended, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c?)+d", boost::regex::extended, "ad", match_default, make_array(0, 2, 1, 1, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){0,0}d", boost::regex::extended, "ad", match_default, make_array(0, 2, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){0,1}d", boost::regex::extended, "ad", match_default, make_array(0, 2, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){0,1}d", boost::regex::extended, "abd", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){0,2}d", boost::regex::extended, "ad", match_default, make_array(0, 2, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){0,2}d", boost::regex::extended, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){0,}d", boost::regex::extended, "ad", match_default, make_array(0, 2, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){0,}d", boost::regex::extended, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){1,1}d", boost::regex::extended, "abd", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){1,2}d", boost::regex::extended, "abd", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){1,2}d", boost::regex::extended, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){1,}d", boost::regex::extended, "abd", match_default, make_array(0, 3, 1, 2, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){1,}d", boost::regex::extended, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){2,2}d", boost::regex::extended, "acbd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){2,2}d", boost::regex::extended, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){2,4}d", boost::regex::extended, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){2,4}d", boost::regex::extended, "abcbd", match_default, make_array(0, 5, 3, 4, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){2,4}d", boost::regex::extended, "abcbcd", match_default, make_array(0, 6, 4, 5, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){2,}d", boost::regex::extended, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|c){2,}d", boost::regex::extended, "abcbd", match_default, make_array(0, 5, 3, 4, -2, -2));
   // perl only:
   TEST_REGEX_SEARCH("a(b|c?)+d", perl, "abcd", match_default, make_array(0, 4, 3, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b+|((c)*))+d", perl, "abd", match_default, make_array(0, 3, 2, 2, 2, 2, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("a(b+|((c)*))+d", perl, "abcd", match_default, make_array(0, 4, 3, 3, 3, 3, 2, 3, -2, -2));
   // posix only:
   TEST_REGEX_SEARCH("a(b|c?)+d", boost::regex::extended, "abcd", match_default, make_array(0, 4, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b|((c)*))+d", boost::regex::extended, "abcd", match_default, make_array(0, 4, 2, 3, 2, 3, 2, 3, -2, -2));
   TEST_REGEX_SEARCH("a(b+|((c)*))+d", boost::regex::extended, "abd", match_default, make_array(0, 3, 1, 2, -1, -1, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("a(b+|((c)*))+d", boost::regex::extended, "abcd", match_default, make_array(0, 4, 2, 3, 2, 3, 2, 3, -2, -2));
   // literals:
   TEST_REGEX_SEARCH("\\**?/{}", literal, "\\**?/{}", match_default, make_array(0, 7, -2, -2));
   TEST_REGEX_SEARCH("\\**?/{}", literal, "\\**?/{", match_default, make_array(-2, -2));
   // try to match C++ syntax elements:
   // line comment:
   TEST_REGEX_SEARCH("//[^\\n]*", perl, "++i //here is a line comment\n", match_default, make_array(4, 28, -2, -2));
   // block comment:
   TEST_REGEX_SEARCH("/\\*([^*]|\\*+[^*/])*\\*+/", perl, "/* here is a block comment */", match_default, make_array(0, 29, 26, 27, -2, -2));
   TEST_REGEX_SEARCH("/\\*([^*]|\\*+[^*/])*\\*+/", perl, "/**/", match_default, make_array(0, 4, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("/\\*([^*]|\\*+[^*/])*\\*+/", perl, "/***/", match_default, make_array(0, 5, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("/\\*([^*]|\\*+[^*/])*\\*+/", perl, "/****/", match_default, make_array(0, 6, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("/\\*([^*]|\\*+[^*/])*\\*+/", perl, "/*****/", match_default, make_array(0, 7, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("/\\*([^*]|\\*+[^*/])*\\*+/", perl, "/*****/*/", match_default, make_array(0, 7, -1, -1, -2, -2));
   // preprossor directives:
   TEST_REGEX_SEARCH("^[[:blank:]]*#([^\\n]*\\\\[[:space:]]+)*[^\\n]*", perl, "#define some_symbol", match_default, make_array(0, 19, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("^[[:blank:]]*#([^\\n]*\\\\[[:space:]]+)*[^\\n]*", perl, "#define some_symbol(x) #x", match_default, make_array(0, 25, -1, -1, -2, -2));
   // try to match C++ syntax elements:
   // line comment:
   TEST_REGEX_SEARCH("//[^\\n]*", boost::regex::extended&~no_escape_in_lists, "++i //here is a line comment\n", match_default, make_array(4, 28, -2, -2));
   // block comment:
   TEST_REGEX_SEARCH("/\\*([^*]|\\*+[^*/])*\\*+/", boost::regex::extended&~no_escape_in_lists, "/* here is a block comment */", match_default, make_array(0, 29, 26, 27, -2, -2));
   TEST_REGEX_SEARCH("/\\*([^*]|\\*+[^*/])*\\*+/", boost::regex::extended&~no_escape_in_lists, "/**/", match_default, make_array(0, 4, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("/\\*([^*]|\\*+[^*/])*\\*+/", boost::regex::extended&~no_escape_in_lists, "/***/", match_default, make_array(0, 5, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("/\\*([^*]|\\*+[^*/])*\\*+/", boost::regex::extended&~no_escape_in_lists, "/****/", match_default, make_array(0, 6, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("/\\*([^*]|\\*+[^*/])*\\*+/", boost::regex::extended&~no_escape_in_lists, "/*****/", match_default, make_array(0, 7, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("/\\*([^*]|\\*+[^*/])*\\*+/", boost::regex::extended&~no_escape_in_lists, "/*****/*/", match_default, make_array(0, 7, -1, -1, -2, -2));
   // preprossor directives:
   TEST_REGEX_SEARCH("^[[:blank:]]*#([^\\n]*\\\\[[:space:]]+)*[^\\n]*", boost::regex::extended&~no_escape_in_lists, "#define some_symbol", match_default, make_array(0, 19, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("^[[:blank:]]*#([^\\n]*\\\\[[:space:]]+)*[^\\n]*", boost::regex::extended&~no_escape_in_lists, "#define some_symbol(x) #x", match_default, make_array(0, 25, -1, -1, -2, -2));
   // perl only:
   TEST_REGEX_SEARCH("^[[:blank:]]*#([^\\n]*\\\\[[:space:]]+)*[^\\n]*", perl, "#define some_symbol(x) \\  \r\n  foo();\\\r\n   printf(#x);", match_default, make_array(0, 53, 30, 42, -2, -2));
   // POSIX leftmost longest checks:
   TEST_REGEX_SEARCH("(aaa)|(\\w+)", boost::regex::extended&~no_escape_in_lists, "a", match_default, make_array(0, 1, -1, -1, 0, 1, -2, -2));
   TEST_REGEX_SEARCH("(aaa)|(\\w+)", boost::regex::extended&~no_escape_in_lists, "aa", match_default, make_array(0, 2, -1, -1, 0, 2, -2, -2));
   TEST_REGEX_SEARCH("(aaa)|(\\w+)", boost::regex::extended&~no_escape_in_lists, "aaa", match_default, make_array(0, 3, 0, 3, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("(aaa)|(\\w+)", boost::regex::extended&~no_escape_in_lists, "aaaa", match_default, make_array(0, 4, -1, -1, 0, 4, -2, -2));
   TEST_REGEX_SEARCH("($)|(\\>)", boost::regex::extended&~no_escape_in_lists, "aaaa", match_default, make_array(4, 4, 4, 4, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("($)|(\\>)", boost::regex::extended&~no_escape_in_lists, "aaaa", match_default|match_not_eol, make_array(4, 4, -1, -1, 4, 4, -2, -2));
   TEST_REGEX_SEARCH("(aaa)(ab)*", boost::regex::extended, "aaaabab", match_default, make_array(0, 7, 0, 3, 5, 7, -2, -2));
}

void test_tricky_cases3()
{
   using namespace boost::regex_constants;
   TEST_REGEX_SEARCH("((0x[[:xdigit:]]+)|([[:digit:]]+))u?((int(8|16|32|64))|L)?", perl, "0xFF", match_default, make_array(0, 4, 0, 4, 0, 4, -1, -1, -1, -1, -1, -1, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("((0x[[:xdigit:]]+)|([[:digit:]]+))u?((int(8|16|32|64))|L)?", perl, "35", match_default, make_array(0, 2, 0, 2, -1, -1, 0, 2, -1, -1, -1, -1, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("((0x[[:xdigit:]]+)|([[:digit:]]+))u?((int(8|16|32|64))|L)?", perl, "0xFFu", match_default, make_array(0, 5, 0, 4, 0, 4, -1, -1, -1, -1, -1, -1, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("((0x[[:xdigit:]]+)|([[:digit:]]+))u?((int(8|16|32|64))|L)?", perl, "0xFFL", match_default, make_array(0, 5, 0, 4, 0, 4, -1, -1, 4, 5, -1, -1, -1, -1, -2, -2));
   TEST_REGEX_SEARCH("((0x[[:xdigit:]]+)|([[:digit:]]+))u?((int(8|16|32|64))|L)?", perl, "0xFFFFFFFFFFFFFFFFuint64", match_default, make_array(0, 24, 0, 18, 0, 18, -1, -1, 19, 24, 19, 24, 22, 24, -2, -2));
   // strings:
   TEST_REGEX_SEARCH("'([^\\\\']|\\\\.)*'", perl, "'\\x3A'", match_default, make_array(0, 6, 4, 5, -2, -2));
   TEST_REGEX_SEARCH("'([^\\\\']|\\\\.)*'", perl, "'\\''", match_default, make_array(0, 4, 1, 3, -2, -2));
   TEST_REGEX_SEARCH("'([^\\\\']|\\\\.)*'", perl, "'\\n'", match_default, make_array(0, 4, 1, 3, -2, -2));
   // posix only:
   TEST_REGEX_SEARCH("^[[:blank:]]*#([^\\n]*\\\\[[:space:]]+)*[^\\n]*", awk, "#define some_symbol(x) \\  \r\n  foo();\\\r\n   printf(#x);", match_default, make_array(0, 53, 28, 42, -2, -2));
   // now try and test some unicode specific characters:
#if !BOOST_WORKAROUND(__BORLANDC__, < 0x560)
   TEST_REGEX_SEARCH_W(L"[[:unicode:]]+", perl, L"a\x0300\x0400z", match_default, make_array(1, 3, -2, -2));
   TEST_REGEX_SEARCH_W(L"[\x10-\xff]", perl, L"\x0300\x0400", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH_W(L"[\01-\05]{5}", perl, L"\x0300\x0400\x0300\x0400\x0300\x0400", match_default, make_array(-2, -2));
#if !BOOST_WORKAROUND(__DECCXX_VER, BOOST_TESTED_AT(60590042))
   TEST_REGEX_SEARCH_W(L"[\x300-\x400]+", perl, L"\x0300\x0400\x0300\x0400\x0300\x0400", match_default, make_array(0, 6, -2, -2));
   TEST_REGEX_SEARCH_W(L"[\\x{300}-\\x{400}]+", perl, L"\x0300\x0400\x0300\x0400\x0300\x0400", match_default, make_array(0, 6, -2, -2));
   TEST_REGEX_SEARCH_W(L"\\x{300}\\x{400}+", perl, L"\x0300\x0400\x0400\x0400\x0400\x0400", match_default, make_array(0, 6, -2, -2));
#endif
#endif
   // finally try some case insensitive matches:
   TEST_REGEX_SEARCH("0123456789@abcdefghijklmnopqrstuvwxyz\\[\\\\\\]\\^_`ABCDEFGHIJKLMNOPQRSTUVWXYZ\\{\\|\\}", perl|icase, "0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}", match_default, make_array(0, 72, -2, -2));
   TEST_REGEX_SEARCH("a", perl|icase, "A", match_default, make_array(0, 1, -2, -2));   
   TEST_REGEX_SEARCH("A", perl|icase, "a", match_default, make_array(0, 1, -2, -2));   
   TEST_REGEX_SEARCH("[abc]+", perl|icase, "abcABC", match_default, make_array(0, 6, -2, -2));   
   TEST_REGEX_SEARCH("[ABC]+", perl|icase, "abcABC", match_default, make_array(0, 6, -2, -2));   
   TEST_REGEX_SEARCH("[a-z]+", perl|icase, "abcABC", match_default, make_array(0, 6, -2, -2));   
   TEST_REGEX_SEARCH("[A-Z]+", perl|icase, "abzANZ", match_default, make_array(0, 6, -2, -2));   
   TEST_REGEX_SEARCH("[a-Z]+", perl|icase, "abzABZ", match_default, make_array(0, 6, -2, -2));   
   TEST_REGEX_SEARCH("[A-z]+", perl|icase, "abzABZ", match_default, make_array(0, 6, -2, -2));   
   TEST_REGEX_SEARCH("[[:lower:]]+", perl|icase, "abyzABYZ", match_default, make_array(0, 8, -2, -2));   
   TEST_REGEX_SEARCH("[[:upper:]]+", perl|icase, "abzABZ", match_default, make_array(0, 6, -2, -2));   
   TEST_REGEX_SEARCH("[[:word:]]+", perl|icase, "abcZZZ", match_default, make_array(0, 6, -2, -2));   
   TEST_REGEX_SEARCH("[[:alpha:]]+", perl|icase, "abyzABYZ", match_default, make_array(0, 8, -2, -2));   
   TEST_REGEX_SEARCH("[[:alnum:]]+", perl|icase, "09abyzABYZ", match_default, make_array(0, 10, -2, -2));   

   // known and suspected bugs:
   TEST_REGEX_SEARCH("\\(", perl, "(", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\)", perl, ")", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\$", perl, "$", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\^", perl, "^", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\.", perl, ".", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\*", perl, "*", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\+", perl, "+", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\?", perl, "?", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\[", perl, "[", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\]", perl, "]", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\|", perl, "|", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\\\", perl, "\\", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("#", perl, "#", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\#", perl, "#", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("a-", perl, "a-", match_default, make_array(0, 2, -2, -2));
   TEST_REGEX_SEARCH("\\-", perl, "-", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\{", perl, "{", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\}", perl, "}", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("0", perl, "0", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("1", perl, "1", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("9", perl, "9", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("b", perl, "b", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("B", perl, "B", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("<", perl, "<", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH(">", perl, ">", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("w", perl, "w", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("W", perl, "W", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("`", perl, "`", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH(" ", perl, " ", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("\\n", perl, "\n", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH(",", perl, ",", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("a", perl, "a", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("f", perl, "f", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("n", perl, "n", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("r", perl, "r", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("t", perl, "t", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("v", perl, "v", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("c", perl, "c", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("x", perl, "x", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH(":", perl, ":", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH("(\\.[[:alnum:]]+){2}", perl, "w.a.b ", match_default, make_array(1, 5, 3, 5, -2, -2));

   // new bugs detected in spring 2003:
   TEST_REGEX_SEARCH("b", perl, "abc", match_default|match_continuous, make_array(-2, -2));
   TEST_REGEX_SEARCH("(?!foo)bar", perl, "foobar", match_default, make_array(3, 6, -2, -2));
   TEST_REGEX_SEARCH("(?!foo)bar", perl, "??bar", match_default, make_array(2, 5, -2, -2));
   TEST_REGEX_SEARCH("(?!foo)bar", perl, "barfoo", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("(?!foo)bar", perl, "bar??", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("(?!foo)bar", perl, "bar", match_default, make_array(0, 3, -2, -2));
   TEST_REGEX_SEARCH("a\\Z", perl, "a\nb", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("()", perl, "abc", match_default, make_array(0, 0, 0, 0, -2, 1, 1, 1, 1, -2, 2, 2, 2, 2, -2, 3, 3, 3, 3, -2, -2));
   TEST_REGEX_SEARCH("^()", perl, "abc", match_default, make_array(0, 0, 0, 0, -2, -2));
   TEST_REGEX_SEARCH("^()+", perl, "abc", match_default, make_array(0, 0, 0, 0, -2, -2));
   TEST_REGEX_SEARCH("^(){1}", perl, "abc", match_default, make_array(0, 0, 0, 0, -2, -2));
   TEST_REGEX_SEARCH("^(){2}", perl, "abc", match_default, make_array(0, 0, 0, 0, -2, -2));
   TEST_REGEX_SEARCH("^((){2})", perl, "abc", match_default, make_array(0, 0, 0, 0, 0, 0, -2, -2));
   TEST_REGEX_SEARCH("()", perl, "", match_default, make_array(0, 0, 0, 0, -2, -2));
   TEST_REGEX_SEARCH("()\\1", perl, "", match_default, make_array(0, 0, 0, 0, -2, -2));
   TEST_REGEX_SEARCH("()\\1", perl, "a", match_default, make_array(0, 0, 0, 0, -2, 1, 1, 1, 1, -2, -2));
   TEST_REGEX_SEARCH("a()\\1b", perl, "ab", match_default, make_array(0, 2, 1, 1, -2, -2));
   TEST_REGEX_SEARCH("a()b\\1", perl, "ab", match_default, make_array(0, 2, 1, 1, -2, -2));
   TEST_REGEX_SEARCH("([a-c]+)\\1", perl, "abcbc", match_default, make_array(1, 5, 1, 3, -2, -2));
   TEST_REGEX_SEARCH(".+abc", perl, "xxxxxxxxyyyyyyyyab", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("(.+)\\1", perl, "abcdxxxyyyxxxyyy", match_default, make_array(4, 16, 4, 10, -2, -2));
   // this should not throw:
   TEST_REGEX_SEARCH("[_]+$", perl, "___________________________________________x", match_default, make_array(-2, -2));
   // bug in V4 code detected 2004/05/12:
   TEST_REGEX_SEARCH("\\l+", perl|icase, "abcXYZ", match_default, make_array(0, 6, -2, -2));
   TEST_REGEX_SEARCH("\\u+", perl|icase, "abcXYZ", match_default, make_array(0, 6, -2, -2));
   TEST_REGEX_SEARCH("(a)(?:b)", perl|nosubs, "ab", match_default, make_array(0, 2, -2, -2));
   // bug reported 2006-09-20:
   TEST_REGEX_SEARCH("(?:\\d{9}.*){2}", perl, "123456789dfsdfsdfsfsdfds123456789b", match_default, make_array(0, 34, -2, -2));
   TEST_REGEX_SEARCH("(?:\\d{9}.*){2}", perl, "123456789dfsdfsdfsfsdfds12345678", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("(?:\\d{9}.*){2}", perl, "123456789dfsdfsdfsfsdfds", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("^(25[0-5]|2[0-4][0-9]|[0-1]?[0-9]?[0-9])(\\.(25[0-5]|2[0-4][0-9]|[0-1]?[0-9]?[0-9])){3}$", perl, "1.2.03", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("^(25[0-5]|2[0-4][0-9]|[0-1]?[0-9]?[0-9])(\\.(25[0-5]|2[0-4][0-9]|[0-1]?[0-9]?[0-9])){3,4}$", perl, "1.2.03", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH("^(25[0-5]|2[0-4][0-9]|[0-1]?[0-9]?[0-9])(\\.(25[0-5]|2[0-4][0-9]|[0-1]?[0-9]?[0-9])){3,4}?$", perl, "1.2.03", match_default, make_array(-2, -2));


   //
   // the strings in the next test case are too long for most compilers to cope with,
   // we have to break them up and call the testing procs directly rather than rely on the macros:
   //
   static const char* big_text = "00001 01 \r\n00002 02 1         2         3         4         5         6"
      "7         8         9         0\r\n00003 03 0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\r\n"
      "00004 04 \r\n00005 05 \r\n00006 06                                                                      "
      "Seite: 0001\r\n00007 07                                                             "
      "StartSeitEEnde: 0001\r\n00008 08                                                             "
      "StartSeiTe Ende: 0001\r\n00009 09                                                             "
      "Start seiteEnde: 0001\r\n00010 10                                                             "
      "28.2.03\r\n00011 11                                                             "
      "Page: 0001\r\n00012 12                                                             "
      "Juhu die Erste: 0001\r\n00013 13                                                             "
      "Es war einmal! 0001\r\n00014 14                               ABCDEFGHIJKLMNOPQRSTUVWXYZ0001\r\n"
      "00015 15                               abcdefghijklmnopqrstuvwxyz0001\r\n"
      "00016 16                               lars.schmeiser@gft.com\r\n00017 17 \r\n"
      "00018 18 \r\n00019 19 \r\n00020 20 \r\n00021 21 1         2         3         4         5         "
      "6         7         8         9         0\r\n"
      "00022 22 0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\r\n"
      "00023 01 \r\n00024 02 1         2         3         4         5         6         7         8         9         0\r\n"
      "00025 03 0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\r\n"
      "00026 04 \r\n00027 05 \r\n00028 06                                                             "
      "Seite: 0002\r\n00029 07                                                             StartSeitEEnde: 0002\r\n"
      "00030 08                                                             "
      "StartSeiTe Ende: 0002\r\n00031 09                                                             "
      "Start seiteEnde: 0002\r\n00032 10                                                             "
      "28.02.2003\r\n00033 11                                                             "
      "Page: 0002\r\n00034 12                                                             "
      "Juhu die Erste: 0002\r\n00035 13                                                             "
      "Es war einmal! 0002\r\n00036 14                               ABCDEFGHIJKLMNOPQRSTUVWXYZ0002\r\n00037 "
      "15                               abcdefghijklmnopqrstuvwxyz0002\r\n00038 16                               "
      "lars.schmeiser@194.1.12.111\r\n00039 17 \r\n00040 18 \r\n00041 19 \r\n"
      "00042 20 \r\n00043 21 1         2         3         4         5         6         7         8         9         0\r\n";
   
   do{
      test_info<char>::set_info(__FILE__, __LINE__, 
         "(.*\\r\\n){3}.* abcdefghijklmnopqrstuvwxyz.*\\r\\n", 
         perl, big_text, match_default|match_not_dot_newline, 
         make_array(753, 1076, 934, 1005, -2, 2143, 2466, 2324, 2395, -2, -2));
      test(char(0), test_regex_search_tag());
   }while(0);
#if !defined(BOOST_NO_WREGEX) && !defined(BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS) && !BOOST_WORKAROUND(BOOST_MSVC, < 1300)
   do{
      std::string st(big_text);
      test_info<wchar_t>::set_info(__FILE__, __LINE__, 
         L"(.*\\r\\n){3}.* abcdefghijklmnopqrstuvwxyz.*\\r\\n", 
         perl, std::wstring(st.begin(), st.end()), match_default|match_not_dot_newline, 
         make_array(753, 1076, 934, 1005, -2, 2143, 2466, 2324, 2395, -2, -2));
      test(char(0), test_regex_search_tag());
   }while(0);
#endif
}

