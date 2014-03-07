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
  *   FILE         test_mfc.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Test code for MFC/ATL strings with Boost.Regex.
  */

//
// We can only build this if we have ATL support:
//
#include <boost/config.hpp>

#ifdef TEST_MFC

#include <boost/regex/mfc.hpp>
#include "test.hpp"
#include "atlstr.h"

#pragma warning(disable:4267)

void test_mfc(const char&, const test_regex_search_tag&)
{
   const std::string& ss = test_info<char>::search_text();
   const std::string& ss2 = test_info<char>::expression();
   CAtlStringA s(ss.c_str(), ss.size());
   CAtlStringA s2(ss2.c_str(), ss2.size());
   boost::regex_constants::match_flag_type opts = test_info<char>::match_options();
   const int* answer_table = test_info<char>::answer_table();
   boost::regex r = boost::make_regex(s2, test_info<char>::syntax_options());
   boost::cmatch what;
   if(boost::regex_search(
      s,
      what,
      r,
      opts))
   {
      test_result(what, s.GetString(), answer_table);
   }
   else if(answer_table[0] >= 0)
   {
      // we should have had a match but didn't:
      BOOST_REGEX_TEST_ERROR("Expected match was not found.", char);
   }
   //
   // regex_match tests:
   //
   if(answer_table[0] < 0)
   {
      if(boost::regex_match(s, r, opts))
      {
         BOOST_REGEX_TEST_ERROR("boost::regex_match found a match when it should not have done so.", char);
      }
   }
   else
   {
      if((answer_table[0] > 0) && boost::regex_match(s, r, opts))
      {
         BOOST_REGEX_TEST_ERROR("boost::regex_match found a match when it should not have done so.", char);
      }
      else if((answer_table[0] == 0) && (answer_table[1] == static_cast<int>(ss.size())))
      {
         if(boost::regex_match(
            s,
            what,
            r,
            opts))
         {
            test_result(what, s.GetString(), answer_table);
            if(!boost::regex_match(s, r, opts))
            {
               // we should have had a match but didn't:
               BOOST_REGEX_TEST_ERROR("Expected match was not found.", char);
            }
         }
         else if(answer_table[0] >= 0)
         {
            // we should have had a match but didn't:
            BOOST_REGEX_TEST_ERROR("Expected match was not found.", char);
         }
      }
   }
   //
   // test regex_iterator:
   //
   boost::cregex_iterator start(boost::make_regex_iterator(s, r, opts)), end;
   boost::cregex_iterator copy(start);
   while(start != end)
   {
      if(start != copy)
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator != comparison.", char);
      }
      if(!(start == copy))
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator == comparison.", char);
      }
      test_result(*start, s.GetString(), answer_table);
      ++start;
      ++copy;
      // move on the answer table to next set of answers;
      if(*answer_table != -2)
         while(*answer_table++ != -2){}
   }
   if(answer_table[0] >= 0)
   {
      // we should have had a match but didn't:
      BOOST_REGEX_TEST_ERROR("Expected match was not found.", char);
   }
   //
   // test regex_token_iterator:
   //
   typedef boost::regex_token_iterator<const char*> token_iterator;
   answer_table = test_info<char>::answer_table();
   //
   // we start by testing sub-expression 0:
   //
   token_iterator tstart(boost::make_regex_token_iterator(s, r, 0, opts)), tend;
   token_iterator tcopy(tstart);
   while(tstart != tend)
   {
      if(tstart != tcopy)
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator != comparison.", char);
      }
      if(!(tstart == tcopy))
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator == comparison.", char);
      }
      test_sub_match(*tstart, s.GetString(), answer_table, 0);
      ++tstart;
      ++tcopy;
      // move on the answer table to next set of answers;
      if(*answer_table != -2)
         while(*answer_table++ != -2){}
   }
   if(answer_table[0] >= 0)
   {
      // we should have had a match but didn't:
      BOOST_REGEX_TEST_ERROR("Expected match was not found.", char);
   }
   //
   // and now field spitting:
   //
   token_iterator tstart2(boost::make_regex_token_iterator(s, r, -1, opts)), tend2;
   token_iterator tcopy2(tstart2);
   int last_end2 = 0;
   answer_table = test_info<char>::answer_table();
   while(tstart2 != tend2)
   {
      if(tstart2 != tcopy2)
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator != comparison.", char);
      }
      if(!(tstart2 == tcopy2))
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator == comparison.", char);
      }
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4244)
#endif
      if(boost::re_detail::distance(s.GetString(), tstart2->first) != last_end2)
      {
         BOOST_REGEX_TEST_ERROR(
            "Error in location of start of field split, found: " 
            << boost::re_detail::distance(s.GetString(), tstart2->first)
            << ", expected: "
            << last_end2
            << ".", char);
      }
      int expected_end = static_cast<int>(answer_table[0] < 0 ? s.GetLength() : answer_table[0]);
      if(boost::re_detail::distance(s.GetString(), tstart2->second) != expected_end)
      {
         BOOST_REGEX_TEST_ERROR(
            "Error in location of end2 of field split, found: "
            << boost::re_detail::distance(s.GetString(), tstart2->second)
            << ", expected: "
            << expected_end
            << ".", char);
      }
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
      last_end2 = answer_table[1];
      ++tstart2;
      ++tcopy2;
      // move on the answer table to next set of answers;
      if(*answer_table != -2)
         while(*answer_table++ != -2){}
   }
   if(answer_table[0] >= 0)
   {
      // we should have had a match but didn't:
      BOOST_REGEX_TEST_ERROR("Expected match was not found.", char);
   }

}

void test_mfc(const wchar_t&, const test_regex_search_tag&)
{
   const std::wstring& ss = test_info<wchar_t>::search_text();
   const std::wstring& ss2 = test_info<wchar_t>::expression();
   CAtlStringW s(ss.c_str(), ss.size());
   CAtlStringW s2(ss2.c_str(), ss2.size());
   boost::regex_constants::match_flag_type opts = test_info<wchar_t>::match_options();
   const int* answer_table = test_info<wchar_t>::answer_table();
   boost::wregex r = boost::make_regex(s2, test_info<wchar_t>::syntax_options());
   boost::wcmatch what;
   if(boost::regex_search(
      s,
      what,
      r,
      opts))
   {
      test_result(what, s.GetString(), answer_table);
   }
   else if(answer_table[0] >= 0)
   {
      // we should have had a match but didn't:
      BOOST_REGEX_TEST_ERROR("Expected match was not found.", wchar_t);
   }
   //
   // regex_match tests:
   //
   if(answer_table[0] < 0)
   {
      if(boost::regex_match(s, r, opts))
      {
         BOOST_REGEX_TEST_ERROR("boost::regex_match found a match when it should not have done so.", wchar_t);
      }
   }
   else
   {
      if((answer_table[0] > 0) && boost::regex_match(s, r, opts))
      {
         BOOST_REGEX_TEST_ERROR("boost::regex_match found a match when it should not have done so.", wchar_t);
      }
      else if((answer_table[0] == 0) && (answer_table[1] == static_cast<int>(ss.size())))
      {
         if(boost::regex_match(
            s,
            what,
            r,
            opts))
         {
            test_result(what, s.GetString(), answer_table);
            if(!boost::regex_match(s, r, opts))
            {
               // we should have had a match but didn't:
               BOOST_REGEX_TEST_ERROR("Expected match was not found.", wchar_t);
            }
         }
         else if(answer_table[0] >= 0)
         {
            // we should have had a match but didn't:
            BOOST_REGEX_TEST_ERROR("Expected match was not found.", wchar_t);
         }
      }
   }
   //
   // test regex_iterator:
   //
   boost::wcregex_iterator start(boost::make_regex_iterator(s, r, opts)), end;
   boost::wcregex_iterator copy(start);
   while(start != end)
   {
      if(start != copy)
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator != comparison.", wchar_t);
      }
      if(!(start == copy))
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator == comparison.", wchar_t);
      }
      test_result(*start, s.GetString(), answer_table);
      ++start;
      ++copy;
      // move on the answer table to next set of answers;
      if(*answer_table != -2)
         while(*answer_table++ != -2){}
   }
   if(answer_table[0] >= 0)
   {
      // we should have had a match but didn't:
      BOOST_REGEX_TEST_ERROR("Expected match was not found.", wchar_t);
   }
   //
   // test regex_token_iterator:
   //
   typedef boost::regex_token_iterator<const wchar_t*> token_iterator;
   answer_table = test_info<wchar_t>::answer_table();
   //
   // we start by testing sub-expression 0:
   //
   token_iterator tstart(boost::make_regex_token_iterator(s, r, 0, opts)), tend;
   token_iterator tcopy(tstart);
   while(tstart != tend)
   {
      if(tstart != tcopy)
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator != comparison.", wchar_t);
      }
      if(!(tstart == tcopy))
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator == comparison.", wchar_t);
      }
      test_sub_match(*tstart, s.GetString(), answer_table, 0);
      ++tstart;
      ++tcopy;
      // move on the answer table to next set of answers;
      if(*answer_table != -2)
         while(*answer_table++ != -2){}
   }
   if(answer_table[0] >= 0)
   {
      // we should have had a match but didn't:
      BOOST_REGEX_TEST_ERROR("Expected match was not found.", wchar_t);
   }
   //
   // and now field spitting:
   //
   token_iterator tstart2(boost::make_regex_token_iterator(s, r, -1, opts)), tend2;
   token_iterator tcopy2(tstart2);
   int last_end2 = 0;
   answer_table = test_info<wchar_t>::answer_table();
   while(tstart2 != tend2)
   {
      if(tstart2 != tcopy2)
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator != comparison.", wchar_t);
      }
      if(!(tstart2 == tcopy2))
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator == comparison.", wchar_t);
      }
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4244)
#endif
      if(boost::re_detail::distance(s.GetString(), tstart2->first) != last_end2)
      {
         BOOST_REGEX_TEST_ERROR(
            "Error in location of start of field split, found: " 
            << boost::re_detail::distance(s.GetString(), tstart2->first)
            << ", expected: "
            << last_end2
            << ".", wchar_t);
      }
      int expected_end = static_cast<int>(answer_table[0] < 0 ? s.GetLength() : answer_table[0]);
      if(boost::re_detail::distance(s.GetString(), tstart2->second) != expected_end)
      {
         BOOST_REGEX_TEST_ERROR(
            "Error in location of end2 of field split, found: "
            << boost::re_detail::distance(s.GetString(), tstart2->second)
            << ", expected: "
            << expected_end
            << ".", wchar_t);
      }
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
      last_end2 = answer_table[1];
      ++tstart2;
      ++tcopy2;
      // move on the answer table to next set of answers;
      if(*answer_table != -2)
         while(*answer_table++ != -2){}
   }
   if(answer_table[0] >= 0)
   {
      // we should have had a match but didn't:
      BOOST_REGEX_TEST_ERROR("Expected match was not found.", wchar_t);
   }

}

void test_mfc(const char&, const test_invalid_regex_tag&)
{
   std::string ss = test_info<char>::expression();
   CAtlStringA s(ss.c_str(), ss.size());
   bool have_catch = false;
   try{
      boost::regex e = boost::make_regex(s, test_info<char>::syntax_options());
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

void test_mfc(const wchar_t&, const test_invalid_regex_tag&)
{
   std::wstring ss = test_info<wchar_t>::expression();
   CAtlStringW s(ss.c_str(), ss.size());
   bool have_catch = false;
   try{
      boost::wregex e = boost::make_regex(s, test_info<wchar_t>::syntax_options());
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
      BOOST_REGEX_TEST_ERROR("Expected a bad_expression exception, but a std::runtime_error instead: " << r.what(), wchar_t);
   }
   catch(const std::exception& r)
   {
      have_catch = true;
      BOOST_REGEX_TEST_ERROR("Expected a bad_expression exception, but a std::exception instead: " << r.what(), wchar_t);
   }
   catch(...)
   {
      have_catch = true;
      BOOST_REGEX_TEST_ERROR("Expected a bad_expression exception, but got an exception of unknown type instead", wchar_t);
   }
   if(!have_catch)
   {
      // oops expected exception was not thrown:
      BOOST_REGEX_TEST_ERROR("Expected an exception, but didn't find one.", wchar_t);
   }
}

void test_mfc(const char&, const test_regex_replace_tag&)
{
   const CStringA expression(test_info<char>::expression().c_str(), test_info<char>::expression().size());
   boost::regex_constants::syntax_option_type syntax_options = test_info<char>::syntax_options();
   try{
      boost::regex r = boost::make_regex(expression, syntax_options);
      if(r.status())
      {
         BOOST_REGEX_TEST_ERROR("Expression did not compile when it should have done, error code = " << r.status(), char);
      }
      const CStringA search_text(test_info<char>::search_text().c_str(), test_info<char>::search_text().size());
      boost::regex_constants::match_flag_type opts = test_info<char>::match_options();
      const CStringA format_string(test_info<char>::format_string().c_str(), test_info<char>::format_string().size());
      const CStringA result_string(test_info<char>::result_string().c_str(), test_info<char>::result_string().size());

      CStringA result = boost::regex_replace(search_text, r, format_string, opts);
      if(result != result_string)
      {
         BOOST_REGEX_TEST_ERROR("regex_replace generated an incorrect string result", char);
      }
   }
   catch(const boost::bad_expression& e)
   {
      BOOST_REGEX_TEST_ERROR("Expression did not compile when it should have done: " << e.what(), char);
   }
   catch(const std::runtime_error& r)
   {
      BOOST_REGEX_TEST_ERROR("Received an unexpected std::runtime_error: " << r.what(), char);
   }
   catch(const std::exception& r)
   {
      BOOST_REGEX_TEST_ERROR("Received an unexpected std::exception: " << r.what(), char);
   }
   catch(...)
   {
      BOOST_REGEX_TEST_ERROR("Received an unexpected exception of unknown type", char);
   }
}

void test_mfc(const wchar_t&, const test_regex_replace_tag&)
{
   const CStringW expression(test_info<wchar_t>::expression().c_str(), test_info<wchar_t>::expression().size());
   boost::regex_constants::syntax_option_type syntax_options = test_info<wchar_t>::syntax_options();
   try{
      boost::wregex r = boost::make_regex(expression, syntax_options);
      if(r.status())
      {
         BOOST_REGEX_TEST_ERROR("Expression did not compile when it should have done, error code = " << r.status(), wchar_t);
      }
      const CStringW search_text(test_info<wchar_t>::search_text().c_str(), test_info<wchar_t>::search_text().size());
      boost::regex_constants::match_flag_type opts = test_info<wchar_t>::match_options();
      const CStringW format_string(test_info<wchar_t>::format_string().c_str(), test_info<wchar_t>::format_string().size());
      const CStringW result_string(test_info<wchar_t>::result_string().c_str(), test_info<wchar_t>::result_string().size());

      CStringW result = boost::regex_replace(search_text, r, format_string, opts);
      if(result != result_string)
      {
         BOOST_REGEX_TEST_ERROR("regex_replace generated an incorrect string result", wchar_t);
      }
   }
   catch(const boost::bad_expression& e)
   {
      BOOST_REGEX_TEST_ERROR("Expression did not compile when it should have done: " << e.what(), wchar_t);
   }
   catch(const std::runtime_error& r)
   {
      BOOST_REGEX_TEST_ERROR("Received an unexpected std::runtime_error: " << r.what(), wchar_t);
   }
   catch(const std::exception& r)
   {
      BOOST_REGEX_TEST_ERROR("Received an unexpected std::exception: " << r.what(), wchar_t);
   }
   catch(...)
   {
      BOOST_REGEX_TEST_ERROR("Received an unexpected exception of unknown type", wchar_t);
   }
}

#else

#include "test.hpp"

void test_mfc(const char&, const test_regex_search_tag&){}
void test_mfc(const wchar_t&, const test_regex_search_tag&){}
void test_mfc(const char&, const test_invalid_regex_tag&){}
void test_mfc(const wchar_t&, const test_invalid_regex_tag&){}
void test_mfc(const char&, const test_regex_replace_tag&){}
void test_mfc(const wchar_t&, const test_regex_replace_tag&){}


#endif
