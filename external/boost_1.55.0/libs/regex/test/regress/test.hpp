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
  *   FILE         test.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Macros for test cases.
  */


#ifndef BOOST_REGEX_REGRESS_TEST_HPP
#define BOOST_REGEX_REGRESS_TEST_HPP

#include <boost/regex.hpp>

#ifdef BOOST_INTEL
// disable Intel's "remarks":
#pragma warning(disable:1418 981 383 1419 7)
#endif

#include <typeinfo>
#include "test_not_regex.hpp"
#include "test_regex_search.hpp"
#include "test_regex_replace.hpp"
#include "test_deprecated.hpp"
#include "test_mfc.hpp"
#include "test_icu.hpp"
#include "test_locale.hpp"

#ifdef TEST_THREADS
#include <boost/thread/once.hpp>
#endif

//
// define test entry proc, this forwards on to the appropriate
// real test:
//
template <class charT, class tagT>
void do_test(const charT& c, const tagT& tag);

template <class charT, class tagT>
void test(const charT& c, const tagT& tag)
{
   do_test(c, tag);
}
// 
// make these non-templates to speed up compilation times:
//
void test(const char&, const test_regex_replace_tag&);
void test(const char&, const test_regex_search_tag&);
void test(const char&, const test_invalid_regex_tag&);

#ifndef BOOST_NO_WREGEX
void test(const wchar_t&, const test_regex_replace_tag&);
void test(const wchar_t&, const test_regex_search_tag&);
void test(const wchar_t&, const test_invalid_regex_tag&);
#endif

template <class Regex>
struct call_once_func
{
   Regex* pregex;
   void operator()()const
   {
      return test_empty(*pregex);
   }
};

template <class charT, class tagT>
void do_test(const charT& c, const tagT& tag)
{
#ifndef BOOST_NO_STD_LOCALE
#if BOOST_WORKAROUND(BOOST_MSVC, <= 1200) && defined(TEST_THREADS)
   // typeid appears to fail in multithreaded environments:
   test_info<charT>::set_typename("");
#else
   test_info<charT>::set_typename(typeid(boost::basic_regex<charT, boost::cpp_regex_traits<charT> >).name());
#endif
   boost::basic_regex<charT, boost::cpp_regex_traits<charT> > e1;
#ifndef TEST_THREADS
   static bool done_empty_test = false;
   if(done_empty_test == false)
   {
      test_empty(e1);
      done_empty_test = true;
   }
#else
   boost::once_flag f = BOOST_ONCE_INIT;
   call_once_func<boost::basic_regex<charT, boost::cpp_regex_traits<charT> > > proc = { &e1 };
   boost::call_once(f, proc);
#endif
   if(test_locale::cpp_locale_state() == test_locale::test_with_locale)
      e1.imbue(test_locale::cpp_locale());
   if(test_locale::cpp_locale_state() != test_locale::no_test)
      test(e1, tag);
#endif
#if !BOOST_WORKAROUND(__BORLANDC__, < 0x560)
#if BOOST_WORKAROUND(BOOST_MSVC, <= 1200) && defined(TEST_THREADS)
   // typeid appears to fail in multithreaded environments:
   test_info<charT>::set_typename("");
#else
   test_info<charT>::set_typename(typeid(boost::basic_regex<charT, boost::c_regex_traits<charT> >).name());
#endif
   boost::basic_regex<charT, boost::c_regex_traits<charT> > e2;
   if(test_locale::c_locale_state() != test_locale::no_test)
      test(e2, tag);
#endif
#if defined(_WIN32) && !defined(BOOST_REGEX_NO_W32)
#if BOOST_WORKAROUND(BOOST_MSVC, <= 1200) && defined(TEST_THREADS)
   // typeid appears to fail in multithreaded environments:
   test_info<charT>::set_typename("");
#else
   test_info<charT>::set_typename(typeid(boost::basic_regex<charT, boost::w32_regex_traits<charT> >).name());
#endif
   boost::basic_regex<charT, boost::w32_regex_traits<charT> > e3;
   if(test_locale::win_locale_state() == test_locale::test_with_locale)
      e3.imbue(test_locale::win_locale());
   if(test_locale::win_locale_state() != test_locale::no_test)
      test(e3, tag);
#endif
   // test old depecated code:
   test_info<charT>::set_typename("Deprecated interfaces");
   if((test_locale::win_locale_state() == test_locale::test_no_locale)
      && (test_locale::c_locale_state() == test_locale::test_no_locale)
      &&(test_locale::cpp_locale_state() == test_locale::test_no_locale))
      test_deprecated(c, tag);
   // test MFC/ATL wrappers:
   test_info<charT>::set_typename("MFC/ATL interfaces");
   if((test_locale::win_locale_state() == test_locale::test_no_locale)
      && (test_locale::c_locale_state() == test_locale::test_no_locale)
      &&(test_locale::cpp_locale_state() == test_locale::test_no_locale))
      test_mfc(c, tag);
   // test ICU code:
   test_info<charT>::set_typename("ICU interfaces");
   test_icu(c, tag);
}

//
// define function to pack args into an array:
//
const int* make_array(int first, ...);


//
// define macros for testing invalid regexes:
//
#define TEST_INVALID_REGEX_N(s, f)\
   do{\
      const char e[] = { s };\
      std::string se(e, sizeof(e) - 1);\
      test_info<char>::set_info(__FILE__, __LINE__, se, f);\
      test(char(0), test_invalid_regex_tag());\
   }while(0)

#ifndef BOOST_NO_WREGEX
#define TEST_INVALID_REGEX_W(s, f)\
   do{\
      const wchar_t e[] = { s };\
      std::wstring se(e, (sizeof(e) / sizeof(wchar_t)) - 1);\
      test_info<wchar_t>::set_info(__FILE__, __LINE__, se, f);\
      test(wchar_t(0), test_invalid_regex_tag());\
   }while(0)
#else
#define TEST_INVALID_REGEX_W(s, f)
#endif

#define TEST_INVALID_REGEX(s, f)\
   TEST_INVALID_REGEX_N(s, f);\
   TEST_INVALID_REGEX_W(BOOST_JOIN(L, s), f)

//
// define macros for testing regex searches:
//
#define TEST_REGEX_SEARCH_N(s, f, t, m, a)\
   do{\
      const char e[] = { s };\
      std::string se(e, sizeof(e) - 1);\
      const char st[] = { t };\
      std::string sst(st, sizeof(st) - 1);\
      test_info<char>::set_info(__FILE__, __LINE__, se, f, sst, m, a);\
      test(char(0), test_regex_search_tag());\
   }while(0)

#ifndef BOOST_NO_WREGEX
#define TEST_REGEX_SEARCH_W(s, f, t, m, a)\
   do{\
      const wchar_t e[] = { s };\
      std::wstring se(e, (sizeof(e) / sizeof(wchar_t)) - 1);\
      const wchar_t st[] = { t };\
      std::wstring sst(st, (sizeof(st) / sizeof(wchar_t)) - 1);\
      test_info<wchar_t>::set_info(__FILE__, __LINE__, se, f, sst, m, a);\
      test(wchar_t(0), test_regex_search_tag());\
   }while(0)
#else
#define TEST_REGEX_SEARCH_W(s, f, t, m, a)
#endif

#define TEST_REGEX_SEARCH(s, f, t, m, a)\
   TEST_REGEX_SEARCH_N(s, f, t, m, a);\
   TEST_REGEX_SEARCH_W(BOOST_JOIN(L, s), f, BOOST_JOIN(L, t), m, a)

#if (defined(__GNUC__) && (__GNUC__ == 3) && (__GNUC_MINOR__ >= 4))
#define TEST_REGEX_SEARCH_L(s, f, t, m, a) TEST_REGEX_SEARCH_W(BOOST_JOIN(L, s), f, BOOST_JOIN(L, t), m, a)
#else
#define TEST_REGEX_SEARCH_L(s, f, t, m, a) TEST_REGEX_SEARCH(s, f, t, m, a)
#endif

//
// define macros for testing regex replaces:
//
#define TEST_REGEX_REPLACE_N(s, f, t, m, fs, r)\
   do{\
      const char e[] = { s };\
      std::string se(e, sizeof(e) - 1);\
      const char st[] = { t };\
      std::string sst(st, sizeof(st) - 1);\
      const char ft[] = { fs };\
      std::string sft(ft, sizeof(ft) - 1);\
      const char rt[] = { r };\
      std::string srt(rt, sizeof(rt) - 1);\
      test_info<char>::set_info(__FILE__, __LINE__, se, f, sst, m, 0, sft, srt);\
      test(char(0), test_regex_replace_tag());\
   }while(0)

#ifndef BOOST_NO_WREGEX
#define TEST_REGEX_REPLACE_W(s, f, t, m, fs, r)\
   do{\
      const wchar_t e[] = { s };\
      std::wstring se(e, (sizeof(e) / sizeof(wchar_t)) - 1);\
      const wchar_t st[] = { t };\
      std::wstring sst(st, (sizeof(st) / sizeof(wchar_t)) - 1);\
      const wchar_t ft[] = { fs };\
      std::wstring sft(ft, (sizeof(ft) / sizeof(wchar_t)) - 1);\
      const wchar_t rt[] = { r };\
      std::wstring srt(rt, (sizeof(rt) / sizeof(wchar_t)) - 1);\
      test_info<wchar_t>::set_info(__FILE__, __LINE__, se, f, sst, m, 0, sft, srt);\
      test(wchar_t(0), test_regex_replace_tag());\
   }while(0)
#else
#define TEST_REGEX_REPLACE_W(s, f, t, m, fs, r)
#endif

#define TEST_REGEX_REPLACE(s, f, t, m, fs, r)\
   TEST_REGEX_REPLACE_N(s, f, t, m, fs, r);\
   TEST_REGEX_REPLACE_W(BOOST_JOIN(L, s), f, BOOST_JOIN(L, t), m, BOOST_JOIN(L, fs), BOOST_JOIN(L, r))

//
// define the test group proceedures:
//
void basic_tests();
void test_simple_repeats();
void test_alt();
void test_sets();
void test_sets2();
void test_anchors();
void test_backrefs();
void test_character_escapes();
void test_assertion_escapes();
void test_tricky_cases();
void test_grep();
void test_replace();
void test_non_greedy_repeats();
void test_non_marking_paren();
void test_partial_match();
void test_forward_lookahead_asserts();
void test_fast_repeats();
void test_fast_repeats2();
void test_tricky_cases2();
void test_independent_subs();
void test_nosubs();
void test_conditionals();
void test_options();
void test_options2();
void test_en_locale();
void test_emacs();
void test_operators();
void test_overloads();
void test_unicode();
void test_pocessive_repeats();
void test_mark_resets();
void test_recursion();

#endif
