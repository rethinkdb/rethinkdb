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
  *   FILE         test_deprecated.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Tests for deprecated interfaces.
  */

#include "test.hpp"
#include <boost/cregex.hpp>

#ifdef BOOST_MSVC
#pragma warning(disable:4267)
#endif

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std{
   using ::atoi;
   using ::wcstol;
}
#endif

int get_posix_compile_options(boost::regex_constants::syntax_option_type opts)
{
   using namespace boost;
   int result = 0;
   switch(opts & regbase::main_option_type)
   {
   case regbase::perl:
      result = (opts & regbase::no_perl_ex) ? REG_EXTENDED : REG_PERL;
      if(opts & (regbase::no_bk_refs|regbase::no_mod_m|regbase::mod_x|regbase::mod_s|regbase::no_mod_s|regbase::no_escape_in_lists|regbase::no_empty_expressions))
         return -1;
      break;
   case regbase::basic:
      result = REG_BASIC;
      if(opts & (regbase::no_char_classes|regbase::no_intervals|regbase::bk_plus_qm|regbase::bk_vbar))
         return -1;
      if((opts & regbase::no_escape_in_lists) == 0)
         return -1;
      break;
   default:
      return -1;
   }

   if(opts & regbase::icase)
      result |= REG_ICASE;
   if(opts & regbase::nosubs)
      result |= REG_NOSUB;
   if(opts & regbase::newline_alt)
      result |= REG_NEWLINE;
   if((opts & regbase::collate) == 0)
      result |= REG_NOCOLLATE;

   return result;
}

int get_posix_match_flags(boost::regex_constants::match_flag_type f)
{
   int result = 0;
   if(f & boost::regex_constants::match_not_bol)
      result |= boost::REG_NOTBOL;
   if(f & boost::regex_constants::match_not_eol)
      result |= boost::REG_NOTEOL;
   if(f & ~(boost::regex_constants::match_not_bol|boost::regex_constants::match_not_eol))
      return -1;
   return result;
}

void test_deprecated(const char&, const test_regex_search_tag&)
{
   const std::string& expression = test_info<char>::expression();
   if(expression.find('\0') != std::string::npos)
      return;
   const std::string& search_text = test_info<char>::search_text();
   if(search_text.find('\0') != std::string::npos)
      return;
   int posix_options = get_posix_compile_options(test_info<char>::syntax_options());
   if(posix_options < 0)
      return;
   int posix_match_options = get_posix_match_flags(test_info<char>::match_options());
   if(posix_match_options < 0)
      return;
   const int* results = test_info<char>::answer_table();

   // OK try and compile the expression:
   boost::regex_tA re;
   if(boost::regcompA(&re, expression.c_str(), posix_options) != 0)
   {
      BOOST_REGEX_TEST_ERROR("Expression : \"" << expression.c_str() << "\" did not compile with the POSIX C API.", char);
      return;
   }
   // try and find the first occurance:
   static const unsigned max_subs = 100;
   boost::regmatch_t matches[max_subs];
   if(boost::regexecA(&re, search_text.c_str(), max_subs, matches, posix_match_options) == 0)
   {
      int i = 0;
      while(results[2*i] != -2)
      {
         if((int)max_subs > i)
         {
            if(results[2*i] != matches[i].rm_so)
            {
               BOOST_REGEX_TEST_ERROR("Mismatch in start of subexpression " << i << " found with the POSIX C API.", char);
            }
            if(results[2*i+1] != matches[i].rm_eo)
            {
               BOOST_REGEX_TEST_ERROR("Mismatch in end of subexpression " << i << " found with the POSIX C API.", char);
            }
         }
         ++i;
      }
   }
   else
   {
      if(results[0] >= 0)
      {
         BOOST_REGEX_TEST_ERROR("Expression : \"" << expression.c_str() << "\" was not found with the POSIX C API.", char);
      }
   }
   // clean up whatever:
   boost::regfreeA(&re);

   //
   // now try the RegEx class:
   //
   if(test_info<char>::syntax_options() & ~boost::regex::icase)
      return;
   try{
      boost::RegEx e(expression, (test_info<char>::syntax_options() & boost::regex::icase) != 0);
      if(e.error_code())
      {
         BOOST_REGEX_TEST_ERROR("Expression did not compile when it should have done, error code = " << e.error_code(), char);
      }
      if(e.Search(search_text, test_info<char>::match_options()))
      {
         int i = 0;
         while(results[i*2] != -2)
         {
            if(e.Matched(i))
            {
               if(results[2*i] != static_cast<int>(e.Position(i)))
               {
                  BOOST_REGEX_TEST_ERROR("Mismatch in start of subexpression " << i << " found with the RegEx class (found " << e.Position(i) << " expected " << results[2*i] << ").", char);
               }
               if(results[2*i+1] != static_cast<int>(e.Position(i) + e.Length(i)))
               {
                  BOOST_REGEX_TEST_ERROR("Mismatch in end of subexpression " << i << " found with the RegEx class (found " << e.Position(i) + e.Length(i) << " expected " << results[2*i+1] << ").", char);
               }
            }
            else
            {
               if(results[2*i] >= 0)
               {
                  BOOST_REGEX_TEST_ERROR("Mismatch in start of subexpression " << i << " found with the RegEx class (found " << e.Position(i) << " expected " << results[2*i] << ").", char);
               }
               if(results[2*i+1] >= 0)
               {
                  BOOST_REGEX_TEST_ERROR("Mismatch in end of subexpression " << i << " found with the RegEx class (found " << e.Position(i) + e.Length(i) << " expected " << results[2*i+1] << ").", char);
               }
            }
            ++i;
         }
      }
      else
      {
         if(results[0] >= 0)
         {
            BOOST_REGEX_TEST_ERROR("Expression : \"" << expression.c_str() << "\" was not found with class RegEx.", char);
         }
      }
   }
   catch(const boost::bad_expression& r)
   {
      BOOST_REGEX_TEST_ERROR("Expression did not compile with RegEx class: " << r.what(), char);
   }
   catch(const std::runtime_error& r)
   {
      BOOST_REGEX_TEST_ERROR("Unexpected std::runtime_error : " << r.what(), char);
   }
   catch(const std::exception& r)
   {
      BOOST_REGEX_TEST_ERROR("Unexpected std::exception: " << r.what(), char);
   }
   catch(...)
   {
      BOOST_REGEX_TEST_ERROR("Unexpected exception of unknown type", char);
   }

}

void test_deprecated(const wchar_t&, const test_regex_search_tag&)
{
#ifndef BOOST_NO_WREGEX
   const std::wstring& expression = test_info<wchar_t>::expression();
   if(expression.find(L'\0') != std::wstring::npos)
      return;
   const std::wstring& search_text = test_info<wchar_t>::search_text();
   if(search_text.find(L'\0') != std::wstring::npos)
      return;
   int posix_options = get_posix_compile_options(test_info<wchar_t>::syntax_options());
   if(posix_options < 0)
      return;
   int posix_match_options = get_posix_match_flags(test_info<wchar_t>::match_options());
   if(posix_match_options < 0)
      return;
   const int* results = test_info<wchar_t>::answer_table();

   // OK try and compile the expression:
   boost::regex_tW re;
   if(boost::regcompW(&re, expression.c_str(), posix_options) != 0)
   {
      BOOST_REGEX_TEST_ERROR("Expression : \"" << expression.c_str() << "\" did not compile with the POSIX C API.", wchar_t);
      return;
   }
   // try and find the first occurance:
   static const unsigned max_subs = 100;
   boost::regmatch_t matches[max_subs];
   if(boost::regexecW(&re, search_text.c_str(), max_subs, matches, posix_match_options) == 0)
   {
      int i = 0;
      while(results[2*i] != -2)
      {
         if((int)max_subs > i)
         {
            if(results[2*i] != matches[i].rm_so)
            {
               BOOST_REGEX_TEST_ERROR("Mismatch in start of subexpression " << i << " found with the POSIX C API.", wchar_t);
            }
            if(results[2*i+1] != matches[i].rm_eo)
            {
               BOOST_REGEX_TEST_ERROR("Mismatch in end of subexpression " << i << " found with the POSIX C API.", wchar_t);
            }
         }
         ++i;
      }
   }
   else
   {
      if(results[0] >= 0)
      {
         BOOST_REGEX_TEST_ERROR("Expression : \"" << expression.c_str() << "\" was not found with the POSIX C API.", wchar_t);
      }
   }
   // clean up whatever:
   boost::regfreeW(&re);
#endif
}

void test_deprecated(const char&, const test_invalid_regex_tag&)
{
   const std::string& expression = test_info<char>::expression();
   if(expression.find('\0') != std::string::npos)
      return;
   int posix_options = get_posix_compile_options(test_info<char>::syntax_options());
   if(posix_options < 0)
      return;

   // OK try and compile the expression:
   boost::regex_tA re;
   int code = boost::regcompA(&re, expression.c_str(), posix_options);
   if(code == 0)
   {
      boost::regfreeA(&re);
      BOOST_REGEX_TEST_ERROR("Expression : \"" << expression.c_str() << "\" unexpectedly compiled with the POSIX C API.", char);
   }
   else
   {
      char buf[100];
      int s = boost::regerrorA(code, &re, 0, 0);
      if(s < 100)
         s = boost::regerrorA(code, &re, buf, 100);
      s = boost::regerrorA(code | boost::REG_ITOA, &re, 0, 0);
      if(s < 100)
      {
         s = boost::regerrorA(code | boost::REG_ITOA, &re, buf, 100);
         re.re_endp = buf;
         s = boost::regerrorA(code | boost::REG_ATOI, &re, buf, 100);
         if(s)
         {
            int code2 = std::atoi(buf);
            if(code2 != code)
            {
               BOOST_REGEX_TEST_ERROR("Got a bad error code from regerrA with REG_ATOI set: ", char);
            }
         }
      }
   }
   //
   // now try the RegEx class:
   //
   if(test_info<char>::syntax_options() & ~boost::regex::icase)
      return;
   bool have_catch = false;
   try{
      boost::RegEx e(expression, (test_info<char>::syntax_options() & boost::regex::icase) != 0);
      if(e.error_code())
         have_catch = true;
   }
   catch(const boost::bad_expression&)
   {
      have_catch = true;
   }
   catch(const std::runtime_error& r)
   {
      have_catch = true;
      BOOST_REGEX_TEST_ERROR("Expected a bad_expression exception, but a std::runtime_error instead: " << r.what(), char);
   }
   catch(const std::exception& r)
   {
      have_catch = true;
      BOOST_REGEX_TEST_ERROR("Expected a bad_expression exception, but a std::exception instead: " << r.what(), char);
   }
   catch(...)
   {
      have_catch = true;
      BOOST_REGEX_TEST_ERROR("Expected a bad_expression exception, but got an exception of unknown type instead", char);
   }
   if(!have_catch)
   {
      // oops expected exception was not thrown:
      BOOST_REGEX_TEST_ERROR("Expected an exception, but didn't find one.", char);
   }
}

void test_deprecated(const wchar_t&, const test_invalid_regex_tag&)
{
#ifndef BOOST_NO_WREGEX
   const std::wstring& expression = test_info<wchar_t>::expression();
   if(expression.find(L'\0') != std::string::npos)
      return;
   int posix_options = get_posix_compile_options(test_info<wchar_t>::syntax_options());
   if(posix_options < 0)
      return;

   // OK try and compile the expression:
   boost::regex_tW re;
   int code = boost::regcompW(&re, expression.c_str(), posix_options);
   if(code == 0)
   {
      boost::regfreeW(&re);
      BOOST_REGEX_TEST_ERROR("Expression : \"" << expression.c_str() << "\" unexpectedly compiled with the POSIX C API.", wchar_t);
   }
   else
   {
      wchar_t buf[100];
      int s = boost::regerrorW(code, &re, 0, 0);
      if(s < 100)
         s = boost::regerrorW(code, &re, buf, 100);
      s = boost::regerrorW(code | boost::REG_ITOA, &re, 0, 0);
      if(s < 100)
      {
         s = boost::regerrorW(code | boost::REG_ITOA, &re, buf, 100);
         re.re_endp = buf;
         s = boost::regerrorW(code | boost::REG_ATOI, &re, buf, 100);
         if(s)
         {
            long code2 = std::wcstol(buf, 0, 10);
            if(code2 != code)
            {
               BOOST_REGEX_TEST_ERROR("Got a bad error code from regerrW with REG_ATOI set: ", char);
            }
         }
      }
   }
#endif
}
