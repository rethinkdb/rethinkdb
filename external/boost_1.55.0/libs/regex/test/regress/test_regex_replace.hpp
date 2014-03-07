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
  *   FILE         test_regex_replace.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares tests for regex search and replace.
  */

#ifndef BOOST_REGEX_REGRESS_REGEX_REPLACE_HPP
#define BOOST_REGEX_REGRESS_REGEX_REPLACE_HPP
#include "info.hpp"

template<class charT, class traits>
void test_regex_replace(boost::basic_regex<charT, traits>& r)
{
   typedef std::basic_string<charT> string_type;
   const string_type& search_text = test_info<charT>::search_text();
   boost::regex_constants::match_flag_type opts = test_info<charT>::match_options();
   const string_type& format_string = test_info<charT>::format_string();
   const string_type& result_string = test_info<charT>::result_string();

   string_type result = boost::regex_replace(search_text, r, format_string, opts);
   if(result != result_string)
   {
      BOOST_REGEX_TEST_ERROR("regex_replace generated an incorrect string result", charT);
   }
}


struct test_regex_replace_tag{};

template<class charT, class traits>
void test(boost::basic_regex<charT, traits>& r, const test_regex_replace_tag&)
{
   const std::basic_string<charT>& expression = test_info<charT>::expression();
   boost::regex_constants::syntax_option_type syntax_options = test_info<charT>::syntax_options();
   try{
      r.assign(expression, syntax_options);
      if(r.status())
      {
         BOOST_REGEX_TEST_ERROR("Expression did not compile when it should have done, error code = " << r.status(), charT);
      }
      test_regex_replace(r);
   }
   catch(const boost::bad_expression& e)
   {
      BOOST_REGEX_TEST_ERROR("Expression did not compile when it should have done: " << e.what(), charT);
   }
   catch(const std::runtime_error& e)
   {
      BOOST_REGEX_TEST_ERROR("Received an unexpected std::runtime_error: " << e.what(), charT);
   }
   catch(const std::exception& e)
   {
      BOOST_REGEX_TEST_ERROR("Received an unexpected std::exception: " << e.what(), charT);
   }
   catch(...)
   {
      BOOST_REGEX_TEST_ERROR("Received an unexpected exception of unknown type", charT);
   }

}

#endif

