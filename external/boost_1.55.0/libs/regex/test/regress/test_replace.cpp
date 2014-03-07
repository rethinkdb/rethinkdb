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

void test_replace()
{
   using namespace boost::regex_constants;
   // start by testing subs:
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$`", "...");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$'", ",,,");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$&", "aaa");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$0", "aaa");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$1", "");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$15", "");
   TEST_REGEX_REPLACE("(a+)b+", perl, "...aaabbb,,,", match_default|format_no_copy, "$1", "aaa");
   TEST_REGEX_REPLACE("[[:digit:]]*", perl, "123ab", match_default|format_no_copy, "<$0>", "<123><><><>");
   TEST_REGEX_REPLACE("[[:digit:]]*", perl, "123ab1", match_default|format_no_copy, "<$0>", "<123><><><1><>");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$`$", "...$");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "(?1x:y)", "(?1x:y)");
   // and now escapes:
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$x", "$x");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "\\a", "\a");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "\\f", "\f");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "\\n", "\n");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "\\r", "\r");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "\\t", "\t");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "\\v", "\v");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "\\", "\\");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "\\x21", "!");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "\\xz", "xz");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "\\x", "x");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "\\x{z}", "x{z}");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "\\x{12b", "x{12b");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "\\x{21}", "!");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "\\c@", "\0");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "\\c", "c");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "\\e", "\x1B");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "\\0101", "A");
   TEST_REGEX_REPLACE("(a+)", perl, "...aaa,,,", match_default|format_no_copy, "\\u$1", "Aaa");
   TEST_REGEX_REPLACE("(a+)", perl, "...aaa,,,", match_default|format_no_copy, "\\U$1", "AAA");
   TEST_REGEX_REPLACE("(a+)", perl, "...aaa,,,", match_default|format_no_copy, "\\U$1\\E$1", "AAAaaa");
   TEST_REGEX_REPLACE("(A+)", perl, "...AAA,,,", match_default|format_no_copy, "\\l$1", "aAA");
   TEST_REGEX_REPLACE("(A+)", perl, "...AAA,,,", match_default|format_no_copy, "\\L$1", "aaa");
   TEST_REGEX_REPLACE("(A+)", perl, "...AAA,,,", match_default|format_no_copy, "\\L$1\\E$1", "aaaAAA");
   TEST_REGEX_REPLACE("\\w+", perl, "fee FOO FAR bAR", match_default|format_perl, "\\L\\u$0", "Fee Foo Far Bar");
   // sed format sequences:
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_sed|format_no_copy, "\\0", "aabb");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_sed|format_no_copy, "\\1", "aa");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_sed|format_no_copy, "\\2", "bb");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_sed|format_no_copy, "&", "aabb");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_sed|format_no_copy, "$", "$");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_sed|format_no_copy, "$1", "$1");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_sed|format_no_copy, "()?:", "()?:");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_sed|format_no_copy, "\\\\", "\\");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_sed|format_no_copy, "\\&", "&");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_sed|format_no_copy, "\\l\\u\\L\\U\\E", "luLUE");

   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_perl|format_no_copy, "$0", "aabb");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_perl|format_no_copy, "$1", "aa");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_perl|format_no_copy, "$2", "bb");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_perl|format_no_copy, "$&", "aabb");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_perl|format_no_copy, "$$", "$");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_perl|format_no_copy, "&", "&");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_perl|format_no_copy, "\\0", "\0");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_perl|format_no_copy, "()?:", "()?:");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,", match_default|format_perl|format_no_copy, "\\0101", "A");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_perl|format_no_copy, "\\1", "aa");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, " ...aabb,,", match_default|format_perl|format_no_copy, "\\2", "bb");

   // move to copying unmatched data:
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_all, "bbb", "...bbb,,,");
   TEST_REGEX_REPLACE("a+(b+)", perl, "...aaabb,,,", match_default|format_all, "$1", "...bb,,,");
   TEST_REGEX_REPLACE("a+(b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "$1", "...bb,,,b*bbb?");
   TEST_REGEX_REPLACE("(a+)|(b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "(?1A)(?2B)", "...AB,,,AB*AB?");
   TEST_REGEX_REPLACE("(a+)|(b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "?1A:B", "...AB,,,AB*AB?");
   TEST_REGEX_REPLACE("(a+)|(b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "(?1A:B)C", "...ACBC,,,ACBC*ACBC?");
   TEST_REGEX_REPLACE("(a+)|(b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "?1:B", "...B,,,B*B?");

   TEST_REGEX_REPLACE("(a+)|(b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "(?{1}A)(?{2}B)", "...AB,,,AB*AB?");
   TEST_REGEX_REPLACE("(a+)|(b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "?{1}A:B", "...AB,,,AB*AB?");
   TEST_REGEX_REPLACE("(a+)|(b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "(?{1}A:B)C", "...ACBC,,,ACBC*ACBC?");
   TEST_REGEX_REPLACE("(a+)|(b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "?{1}:B", "...B,,,B*B?");
   TEST_REGEX_REPLACE("(?<one>a+)|(?<two>b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "(?{one}A)(?{two}B)", "...AB,,,AB*AB?");
   TEST_REGEX_REPLACE("(?<one>a+)|(?<two>b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "?{one}A:B", "...AB,,,AB*AB?");
   TEST_REGEX_REPLACE("(?<one>a+)|(?<two>b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "(?{one}A:B)C", "...ACBC,,,ACBC*ACBC?");
   TEST_REGEX_REPLACE("(?<one>a+)|(?<two>b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "?{one}:B", "...B,,,B*B?");

   // move to copying unmatched data, but replace first occurance only:
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_all|format_first_only, "bbb", "...bbb,,,");
   TEST_REGEX_REPLACE("a+(b+)", perl, "...aaabb,,,", match_default|format_all|format_first_only, "$1", "...bb,,,");
   TEST_REGEX_REPLACE("a+(b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all|format_first_only, "$1", "...bb,,,ab*abbb?");
   TEST_REGEX_REPLACE("(a+)|(b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all|format_first_only, "(?1A)(?2B)", "...Abb,,,ab*abbb?");
   TEST_REGEX_REPLACE("(a+)|(b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "?1A", "...A,,,A*A?");
   TEST_REGEX_REPLACE("(a+)|(b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "?1:B", "...B,,,B*B?");
   TEST_REGEX_REPLACE("(a+)|(b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "(?1A:(?2B))", "...AB,,,AB*AB?");
   TEST_REGEX_REPLACE("X", literal, "XX", match_default, "Y", "YY");
   TEST_REGEX_REPLACE("(a+)|(b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "(?", "...??,,,??*???");
   TEST_REGEX_REPLACE("(a+)|(b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "(?a", "...?a?a,,,?a?a*?a?a?");
   TEST_REGEX_REPLACE("(a+)|(b+)", perl, "...aaabb,,,ab*abbb?", match_default|format_all, "(?1A:(?2B))abc", "...AabcBabc,,,AabcBabc*AabcBabc?");
   TEST_REGEX_REPLACE("a+(b+)", perl, "...aaabb,,,", match_default|format_all|format_no_copy, "(?2abc:def)", "def");
   TEST_REGEX_REPLACE("a+(b+)", perl, "...aaabb,,,", match_default|format_all|format_no_copy, "(?1abc:def)", "abc");
   TEST_REGEX_REPLACE("a+(b+)", perl, "...aaabb,,,", match_default|format_perl|format_no_copy, "(?1abc:def)", "(?1abc:def)");
   TEST_REGEX_REPLACE("a+(b+)", perl, "...", match_default|format_perl, "(?1abc:def)", "...");
   TEST_REGEX_REPLACE("a+(b+)", perl, "...", match_default|format_perl|format_no_copy, "(?1abc:def)", "");
   // probe bug reports and other special cases:
   TEST_REGEX_REPLACE("([^\\d]+).*", normal|icase, "tesd 999 test", match_default|format_all, "($1)replace", "tesd replace");
   TEST_REGEX_REPLACE("(a)(b)", perl, "ab", match_default|format_all, "$1:$2", "a:b");
   TEST_REGEX_REPLACE("(a(c)?)|(b)", perl, "acab", match_default|format_all, "(?1(?2(C:):A):B:)", "C:AB:");
   TEST_REGEX_REPLACE("x", icase, "xx", match_default|format_all, "a", "aa");
   TEST_REGEX_REPLACE("x", basic|icase, "xx", match_default|format_all, "a", "aa");
   TEST_REGEX_REPLACE("x", boost::regex::extended|icase, "xx", match_default|format_all, "a", "aa");
   TEST_REGEX_REPLACE("x", emacs|icase, "xx", match_default|format_all, "a", "aa");
   TEST_REGEX_REPLACE("x", literal|icase, "xx", match_default|format_all, "a", "aa");
   // literals:
   TEST_REGEX_REPLACE("(a(c)?)|(b)", perl, "acab", match_default|format_literal, "\\&$", "\\&$\\&$\\&$");
   // Bracketed sub expressions:
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default, "$", "...$,,,");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default, "${", "...${,,,");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default, "${2", "...${2,,,");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default, "${23", "...${23,,,");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default, "${d}", "...${d},,,");
   TEST_REGEX_REPLACE("(a+)", perl, "...aaa,,,", match_default, "/${1}/", ".../aaa/,,,");
   TEST_REGEX_REPLACE("(a+)", perl, "...aaa,,,", match_default, "/${10}/", "...//,,,");
   TEST_REGEX_REPLACE("((((((((((a+))))))))))", perl, "...aaa,,,", match_default, "/${10}/", ".../aaa/,,,");
   TEST_REGEX_REPLACE("(a+)", perl, "...aaa,,,", match_default, "/${1}0/", ".../aaa0/,,,");

   // New Perl style operators:
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$MATCH", "aaa");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "${MATCH}", "aaa");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "${^MATCH}", "aaa");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$MATC", "$MATC");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "${MATCH", "${MATCH");

   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$PREMATCH", "...");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "${PREMATCH}", "...");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "${^PREMATCH}", "...");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$PREMATC", "$PREMATC");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "${PREMATCH", "${PREMATCH");

   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$POSTMATCH", ",,,");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "${POSTMATCH}", ",,,");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "${^POSTMATCH}", ",,,");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$POSTMATC", "$POSTMATC");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "${POSTMATCH", "${POSTMATCH");

   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$LAST_PAREN_MATCH", "");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$LAST_PAREN_MATC", "$LAST_PAREN_MATC");
   TEST_REGEX_REPLACE("(a+)", perl, "...aaa,,,", match_default|format_no_copy, "$LAST_PAREN_MATCH", "aaa");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, "...aaabb,,,", match_default|format_no_copy, "$LAST_PAREN_MATCH", "bb");

   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$+", "");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$+foo", "foo");
   TEST_REGEX_REPLACE("(a+)", perl, "...aaa,,,", match_default|format_no_copy, "$+", "aaa");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, "...aaabb,,,", match_default|format_no_copy, "$+foo", "bbfoo");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, "...aaabb,,,", match_default|format_no_copy, "$+{", "bb{");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, "...aaabb,,,", match_default|format_no_copy, "$+{foo", "bb{foo");

   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$LAST_SUBMATCH_RESULT", "");
   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$LAST_SUBMATCH_RESUL", "$LAST_SUBMATCH_RESUL");
   TEST_REGEX_REPLACE("(a+)", perl, "...aaa,,,", match_default|format_no_copy, "$LAST_SUBMATCH_RESULT", "aaa");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, "...aaabb,,,", match_default|format_no_copy, "$LAST_SUBMATCH_RESULT", "bb");
   TEST_REGEX_REPLACE("(a+)|(b+)", perl, "...aaa,,,", match_default|format_no_copy, "$LAST_SUBMATCH_RESULT", "aaa");

   TEST_REGEX_REPLACE("a+", perl, "...aaa,,,", match_default|format_no_copy, "$^N", "");
   TEST_REGEX_REPLACE("(a+)", perl, "...aaa,,,", match_default|format_no_copy, "$^N", "aaa");
   TEST_REGEX_REPLACE("(a+)(b+)", perl, "...aaabb,,,", match_default|format_no_copy, "$^N", "bb");
   TEST_REGEX_REPLACE("(a+)|(b+)", perl, "...aaa,,,", match_default|format_no_copy, "$^N", "aaa");

   TEST_REGEX_REPLACE("(?<one>a+)(?<two>b+)", perl, " ...aabb,,", match_default|format_no_copy, "$&", "aabb");
   TEST_REGEX_REPLACE("(?<one>a+)(?<two>b+)", perl, " ...aabb,,", match_default|format_no_copy, "$1", "aa");
   TEST_REGEX_REPLACE("(?<one>a+)(?<two>b+)", perl, " ...aabb,,", match_default|format_no_copy, "$2", "bb");
   TEST_REGEX_REPLACE("(?<one>a+)(?<two>b+)", perl, " ...aabb,,", match_default|format_no_copy, "d$+{one}c", "daac");
   TEST_REGEX_REPLACE("(?<one>a+)(?<two>b+)", perl, " ...aabb,,", match_default|format_no_copy, "c$+{two}d", "cbbd");

   TEST_REGEX_REPLACE("(?<one>a)(?<one>b)(c)", perl, " ...abc,,", match_default, "$1.$2.$3.$+{one}", " ...a.b.c.a,,");
   TEST_REGEX_REPLACE("(?:(?<one>a)|(?<one>b))", perl, " ...a,,", match_default, "$1.$2.$+{one}", " ...a..a,,");
   TEST_REGEX_REPLACE("(?:(?<one>a)|(?<one>b))", perl, " ...b,,", match_default, "$1.$2.$+{one}", " ....b.b,,");
   TEST_REGEX_REPLACE("(?:(?<one>a)(?<one>b))", perl, " ...ab,,", match_default, "$1.$2.$+{one}", " ...a.b.a,,");

   // See https://svn.boost.org/trac/boost/ticket/589
   TEST_REGEX_REPLACE("(a*)", perl, "aabb", match_default, "{$1}", "{aa}{}b{}b{}");
   TEST_REGEX_REPLACE("(a*)", extended, "aabb", match_default, "{$1}", "{aa}{}b{}b{}");
   TEST_REGEX_REPLACE("(a*)", extended, "aabb", match_default|match_posix, "{$1}", "{aa}b{}b{}");
}

