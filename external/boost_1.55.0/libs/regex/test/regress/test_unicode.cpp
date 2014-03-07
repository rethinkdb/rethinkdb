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
  *   FILE         test_unicode.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Unicode specific tests (requires ICU).
  */

#include <boost/regex/config.hpp>
#ifdef BOOST_HAS_ICU
#include "test.hpp"

#ifdef BOOST_MSVC
#pragma warning(disable:4127)
#endif

#ifndef BOOST_NO_STD_WSTRING

#define TEST_REGEX_SEARCH_U(s, f, t, m, a)\
   do{\
      const wchar_t e[] = { s };\
      std::wstring se(e, (sizeof(e) / sizeof(wchar_t)) - 1);\
      const wchar_t st[] = { t };\
      std::wstring sst(st, (sizeof(st) / sizeof(wchar_t)) - 1);\
      test_info<wchar_t>::set_info(__FILE__, __LINE__, se, f, sst, m, a);\
      test_icu(wchar_t(0), test_regex_search_tag());\
   }while(0)

#define TEST_REGEX_CLASS_U(classname, character)\
   TEST_REGEX_SEARCH_U(\
      L"[[:" BOOST_JOIN(L, BOOST_STRINGIZE(classname)) L":]]",\
      perl, \
      BOOST_JOIN(L, \
         BOOST_STRINGIZE(\
            BOOST_JOIN(\x, character))), \
      match_default, \
      make_array(0, 1, -2, -2))

#else

#define TEST_REGEX_SEARCH_U(s, f, t, m, a)
#define TEST_REGEX_CLASS_U(classname, character)

#endif

void test_unicode()
{
   using namespace boost::regex_constants;

   TEST_REGEX_CLASS_U(L*, 3108);
   TEST_REGEX_CLASS_U(Letter, 3108);
   TEST_REGEX_CLASS_U(Lu, 2145);
   TEST_REGEX_CLASS_U(Uppercase Letter, 2145);
   TEST_REGEX_CLASS_U(Ll, 2146);
   TEST_REGEX_CLASS_U(Lowercase Letter, 2146);
   TEST_REGEX_CLASS_U(Lt, 1FFC);
   TEST_REGEX_CLASS_U(Titlecase Letter, 1FFC);
   TEST_REGEX_CLASS_U(Lm, 1D61);
   TEST_REGEX_CLASS_U(Modifier Letter, 1D61);
   TEST_REGEX_CLASS_U(Lo, 1974);
   TEST_REGEX_CLASS_U(Other Letter, 1974);
   TEST_REGEX_CLASS_U(M*, 20EA);
   TEST_REGEX_CLASS_U(Mark, 20EA);
   TEST_REGEX_CLASS_U(Mn, 20EA);
   TEST_REGEX_CLASS_U(Non-Spacing Mark, 20EA);
   TEST_REGEX_CLASS_U(Mc, 1938);
   TEST_REGEX_CLASS_U(Spacing Combining Mark, 1938);
   TEST_REGEX_CLASS_U(Me, 0488);
   TEST_REGEX_CLASS_U(Enclosing Mark, 0488);
   TEST_REGEX_CLASS_U(N*, 0669);
   TEST_REGEX_CLASS_U(Number, 0669);
   TEST_REGEX_CLASS_U(Nd, 0669);
   TEST_REGEX_CLASS_U(Decimal Digit Number, 0669);
   TEST_REGEX_CLASS_U(Nl, 303A);
   TEST_REGEX_CLASS_U(Letter Number, 303A);
   TEST_REGEX_CLASS_U(No, 2793);
   TEST_REGEX_CLASS_U(Other Number, 2793);

   TEST_REGEX_CLASS_U(S*, 2144);
   TEST_REGEX_CLASS_U(Symbol, 2144);
   TEST_REGEX_CLASS_U(Sm, 2144);
   TEST_REGEX_CLASS_U(Math Symbol, 2144);
   TEST_REGEX_CLASS_U(Sc, 20B1);
   TEST_REGEX_CLASS_U(Currency Symbol, 20B1);
   TEST_REGEX_CLASS_U(Sk, 1FFE);
   TEST_REGEX_CLASS_U(Modifier Symbol, 1FFE);
   TEST_REGEX_CLASS_U(So, 19FF);
   TEST_REGEX_CLASS_U(Other Symbol, 19FF);

   TEST_REGEX_CLASS_U(P*, 005F);
   TEST_REGEX_CLASS_U(Punctuation, 005F);
   TEST_REGEX_CLASS_U(Pc, 005F);
   TEST_REGEX_CLASS_U(Connector Punctuation, 005F);
   TEST_REGEX_CLASS_U(Pd, 002D);
   TEST_REGEX_CLASS_U(Dash Punctuation, 002D);
   TEST_REGEX_CLASS_U(Ps, 0028);
   TEST_REGEX_CLASS_U(Open Punctuation, 0028);
   TEST_REGEX_CLASS_U(Pe, FF63);
   TEST_REGEX_CLASS_U(Close Punctuation, FF63);
   TEST_REGEX_CLASS_U(Pi, 2039);
   TEST_REGEX_CLASS_U(Initial Punctuation, 2039);
   TEST_REGEX_CLASS_U(Pf, 203A);
   TEST_REGEX_CLASS_U(Final Punctuation, 203A);
   TEST_REGEX_CLASS_U(Po, 2038);
   TEST_REGEX_CLASS_U(Other Punctuation, 2038);

   TEST_REGEX_CLASS_U(Z*, 202F);
   TEST_REGEX_CLASS_U(Separator, 202F);
   TEST_REGEX_CLASS_U(Zs, 202F);
   TEST_REGEX_CLASS_U(Space Separator, 202F);
   TEST_REGEX_CLASS_U(Zl, 2028);
   TEST_REGEX_CLASS_U(Line Separator, 2028);
   TEST_REGEX_CLASS_U(Zp, 2029);
   TEST_REGEX_CLASS_U(Paragraph Separator, 2029);
#if !BOOST_WORKAROUND(BOOST_MSVC, < 1300)
   // Some tests have to be disabled for VC6 because the compiler
   // mangles the string literals...
   TEST_REGEX_CLASS_U(C*, 009F);
   TEST_REGEX_CLASS_U(Other, 009F);
   TEST_REGEX_CLASS_U(Cc, 009F);
   TEST_REGEX_CLASS_U(Control, 009F);
#endif
   TEST_REGEX_CLASS_U(Cf, FFFB);
   TEST_REGEX_CLASS_U(Format, FFFB);
   //TEST_REGEX_CLASS_U(Cs, DC00);
   //TEST_REGEX_CLASS_U(Surrogate, DC00);
   TEST_REGEX_CLASS_U(Co, F8FF);
   TEST_REGEX_CLASS_U(Private Use, F8FF);
   TEST_REGEX_CLASS_U(Cn, FFFF);
   TEST_REGEX_CLASS_U(Not Assigned, FFFF);
   TEST_REGEX_CLASS_U(Any, 2038);
   TEST_REGEX_CLASS_U(Assigned, 2038);
   TEST_REGEX_CLASS_U(ASCII, 7f);
   TEST_REGEX_SEARCH_U(L"[[:Assigned:]]", perl, L"\xffff", match_default, make_array(-2, -2));
   TEST_REGEX_SEARCH_U(L"[[:ASCII:]]", perl, L"\x80", match_default, make_array(-2, -2));

   TEST_REGEX_SEARCH_U(L"\\N{KHMER DIGIT SIX}", perl, L"\x17E6", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH_U(L"\\N{MODIFIER LETTER LOW ACUTE ACCENT}", perl, L"\x02CF", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH_U(L"\\N{SUPERSCRIPT ONE}", perl, L"\x00B9", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH_U(L"[\\N{KHMER DIGIT SIX}]", perl, L"\x17E6", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH_U(L"[\\N{MODIFIER LETTER LOW ACUTE ACCENT}]", perl, L"\x02CF", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH_U(L"[\\N{SUPERSCRIPT ONE}]", perl, L"\x00B9", match_default, make_array(0, 1, -2, -2));
   TEST_REGEX_SEARCH_U(L"\\N{CJK UNIFIED IDEOGRAPH-7FED}", perl, L"\x7FED", match_default, make_array(0, 1, -2, -2));
#if !BOOST_WORKAROUND(BOOST_MSVC, < 1300)
   // Some tests have to be disabled for VC6 because the compiler
   // mangles the string literals...
   TEST_REGEX_SEARCH_U(L"\\w+", perl, L" e\x301" L"coute ", match_default, make_array(1, 8, -2, -2));

   TEST_REGEX_SEARCH_U(L"^", perl, L" \x2028 \x2029 \x000D\x000A \x000A \x000C \x000D \x0085 ", 
      match_default | match_not_bol, make_array(2, 2, -2, 4, 4, -2, 7, 7, -2, 9, 9, -2, 11, 11, -2, 13, 13, -2, 15, 15, -2, -2));
   TEST_REGEX_SEARCH_U(L"$", perl, L" \x2028 \x2029 \x000D\x000A \x000A \x000C \x000D \x0085 ", 
      match_default | match_not_eol, make_array(1, 1, -2, 3, 3, -2, 5, 5, -2, 8, 8, -2, 10, 10, -2, 12, 12, -2, 14, 14, -2, -2));
   TEST_REGEX_SEARCH_U(L".", perl, L" \x2028\x2029\x000D\x000A\x000A\x000C\x000D\x0085 ", 
      match_default | match_not_dot_newline, make_array(0, 1, -2, 9, 10, -2, -2));
#endif
}

#else
void test_unicode(){}
#endif
