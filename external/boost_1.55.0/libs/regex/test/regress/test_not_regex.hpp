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
  *   FILE         test_not_regex.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares tests for invalid regexes.
  */


#ifndef BOOST_REGEX_REGRESS_TEST_NOT_REGEX_HPP
#define BOOST_REGEX_REGRESS_TEST_NOT_REGEX_HPP
#include "info.hpp"
//
// this file implements a test for a regular expression that should not compile:
//
struct test_invalid_regex_tag{};

template<class charT, class traits>
void test_empty(boost::basic_regex<charT, traits>& r)
{
   if(!r.empty())
   {
      BOOST_REGEX_TEST_ERROR("Invalid value returned from basic_regex<>::empty().", charT);
   }
   if(r.size())
   {
      BOOST_REGEX_TEST_ERROR("Invalid value returned from basic_regex<>::size().", charT);
   }
   if(r.str().size())
   {
      BOOST_REGEX_TEST_ERROR("Invalid value returned from basic_regex<>::str().", charT);
   }
   if(r.begin() != r.end())
   {
      BOOST_REGEX_TEST_ERROR("Invalid value returned from basic_regex<>::begin().", charT);
   }
   if(r.status() == 0)
   {
      BOOST_REGEX_TEST_ERROR("Invalid value returned from basic_regex<>::status().", charT);
   }
   if(r.begin() != r.end())
   {
      BOOST_REGEX_TEST_ERROR("Invalid value returned from basic_regex<>::begin().", charT);
   }
}

template<class charT, class traits>
void test(boost::basic_regex<charT, traits>& r, const test_invalid_regex_tag&)
{
   const std::basic_string<charT>& expression = test_info<charT>::expression();
   boost::regex_constants::syntax_option_type syntax_options = test_info<charT>::syntax_options();
   //
   // try it with exceptions disabled first:
   //
   try
   {
      if(0 == r.assign(expression, syntax_options | boost::regex_constants::no_except).status())
      {
         BOOST_REGEX_TEST_ERROR("Expression compiled when it should not have done so.", charT);
      }
      test_empty(r);
   }
   catch(...)
   {
      BOOST_REGEX_TEST_ERROR("Unexpected exception thrown.", charT);
   }
   //
   // now try again with exceptions:
   //
   bool have_catch = false;
   try{
      r.assign(expression, syntax_options);
#ifdef BOOST_NO_EXCEPTIONS
      if(r.status())
         have_catch = true;
#endif
   }
   catch(const boost::bad_expression&)
   {
      have_catch = true;
      test_empty(r);
   }
   catch(const std::runtime_error& e)
   {
      have_catch = true;
      BOOST_REGEX_TEST_ERROR("Expected a bad_expression exception, but a std::runtime_error instead: " << e.what(), charT);
   }
   catch(const std::exception& e)
   {
      have_catch = true;
      BOOST_REGEX_TEST_ERROR("Expected a bad_expression exception, but a std::exception instead: " << e.what(), charT);
   }
   catch(...)
   {
      have_catch = true;
      BOOST_REGEX_TEST_ERROR("Expected a bad_expression exception, but got an exception of unknown type instead", charT);
   }
   if(!have_catch)
   {
      // oops expected exception was not thrown:
      BOOST_REGEX_TEST_ERROR("Expected an exception, but didn't find one.", charT);
   }

}



#endif
