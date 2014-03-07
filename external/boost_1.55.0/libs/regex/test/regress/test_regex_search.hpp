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
  *   FILE         test_regex_search.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares tests for regex search and iteration.
  */

#ifndef BOOST_REGEX_REGRESS_REGEX_SEARCH_HPP
#define BOOST_REGEX_REGRESS_REGEX_SEARCH_HPP
#include "info.hpp"
#ifdef TEST_ROPE
#include <rope>
#endif
//
// this file implements a test for a regular expression that should compile,
// followed by a search for that expression:
//
struct test_regex_search_tag{};

template <class BidirectionalIterator>
void test_sub_match(const boost::sub_match<BidirectionalIterator>& sub, BidirectionalIterator base, const int* answer_table, int i, bool recurse = true)
{
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4244)
#endif
   if(recurse)
   {
      boost::sub_match<BidirectionalIterator> copy(sub);
      test_sub_match(copy, base, answer_table, i, false);
   }
   typedef typename boost::sub_match<BidirectionalIterator>::value_type charT;
   if((sub.matched == 0) 
      && 
         !((i == 0)
          && (test_info<charT>::match_options() & boost::match_partial)) )
   {
      if(answer_table[2*i] >= 0)
      {
         BOOST_REGEX_TEST_ERROR(
            "Sub-expression " << i 
            << " was not matched when it should have been.", charT);
      }
   }
   else
   {
      if(boost::re_detail::distance(base, sub.first) != answer_table[2*i])
      {
         BOOST_REGEX_TEST_ERROR(
            "Error in start location of sub-expression " 
            << i << ", found " << boost::re_detail::distance(base, sub.first) 
            << ", expected " << answer_table[2*i] << ".", charT);
      }
      if(boost::re_detail::distance(base, sub.second) != answer_table[1+ 2*i])
      {
         BOOST_REGEX_TEST_ERROR(
            "Error in end location of sub-expression " 
            << i << ", found " << boost::re_detail::distance(base, sub.second) 
            << ", expected " << answer_table[1 + 2*i] << ".", charT);
      }
   }
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
}

template <class BidirectionalIterator, class Allocator>
void test_result(const boost::match_results<BidirectionalIterator, Allocator>& what, BidirectionalIterator base, const int* answer_table, bool recurse = true)
{
   if(recurse)
   {
      boost::match_results<BidirectionalIterator, Allocator> copy(what);
      test_result(copy, base, answer_table, false);
      boost::match_results<BidirectionalIterator, Allocator> s;
      s.swap(copy);
      test_result(s, base, answer_table, false);
      boost::match_results<BidirectionalIterator, Allocator> s2;
      s2 = what;
      test_result(s2, base, answer_table, false);
   }
   for(unsigned i = 0; i < what.size(); ++i)
   {
      test_sub_match(what[i], base, answer_table, i);
   }
}

template<class charT, class traits>
void test_simple_search(boost::basic_regex<charT, traits>& r)
{
   typedef typename std::basic_string<charT>::const_iterator const_iterator;
   const std::basic_string<charT>& search_text = test_info<charT>::search_text();
   boost::regex_constants::match_flag_type opts = test_info<charT>::match_options();
   const int* answer_table = test_info<charT>::answer_table();
   boost::match_results<const_iterator> what;
   if(boost::regex_search(
      search_text.begin(),
      search_text.end(),
      what,
      r,
      opts))
   {
      test_result(what, search_text.begin(), answer_table);
      // setting match_any should have no effect on the result returned:
      if(!boost::regex_search(
         search_text.begin(),
         search_text.end(),
         r,
         opts|boost::regex_constants::match_any))
      {
         BOOST_REGEX_TEST_ERROR("Expected match was not found when using the match_any flag.", charT);
      }
   }
   else
   {
      if(answer_table[0] >= 0)
      {
         // we should have had a match but didn't:
         BOOST_REGEX_TEST_ERROR("Expected match was not found.", charT);
      }
      // setting match_any should have no effect on the result returned:
      else if(boost::regex_search(
         search_text.begin(),
         search_text.end(),
         r,
         opts|boost::regex_constants::match_any))
      {
         BOOST_REGEX_TEST_ERROR("Unexpected match was found when using the match_any flag.", charT);
      }
   }
#ifdef TEST_ROPE
   std::rope<charT> rsearch_text;
   for(unsigned i = 0; i < search_text.size(); ++i)
   {
      std::rope<charT> c(search_text[i]);
      if(++i != search_text.size())
      {
         c.append(search_text[i]);
         if(++i != search_text.size())
         {
            c.append(search_text[i]);
         }
      }
      rsearch_text.append(c);
   }
   boost::match_results<std::rope<charT>::const_iterator> rwhat;
   if(boost::regex_search(
      rsearch_text.begin(),
      rsearch_text.end(),
      rwhat,
      r,
      opts))
   {
      test_result(rwhat, rsearch_text.begin(), answer_table);
   }
   else
   {
      if(answer_table[0] >= 0)
      {
         // we should have had a match but didn't:
         BOOST_REGEX_TEST_ERROR("Expected match was not found.", charT);
      }
   }
#endif
}

template<class charT, class traits>
void test_regex_iterator(boost::basic_regex<charT, traits>& r)
{
   typedef typename std::basic_string<charT>::const_iterator const_iterator;
   typedef boost::regex_iterator<const_iterator, charT, traits> test_iterator;
   const std::basic_string<charT>& search_text = test_info<charT>::search_text();
   boost::regex_constants::match_flag_type opts = test_info<charT>::match_options();
   const int* answer_table = test_info<charT>::answer_table();
   test_iterator start(search_text.begin(), search_text.end(), r, opts), end;
   test_iterator copy(start);
   const_iterator last_end = search_text.begin();
   while(start != end)
   {
      if(start != copy)
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator != comparison.", charT);
      }
      if(!(start == copy))
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator == comparison.", charT);
      }
      test_result(*start, search_text.begin(), answer_table);
      // test $` and $' :
      if(start->prefix().first != last_end)
      {
         BOOST_REGEX_TEST_ERROR("Incorrect position for start of $`", charT);
      }
      if(start->prefix().second != (*start)[0].first)
      {
         BOOST_REGEX_TEST_ERROR("Incorrect position for end of $`", charT);
      }
      if(start->prefix().matched != (start->prefix().first != start->prefix().second))
      {
         BOOST_REGEX_TEST_ERROR("Incorrect position for matched member of $`", charT);
      }
      if(start->suffix().first != (*start)[0].second)
      {
         BOOST_REGEX_TEST_ERROR("Incorrect position for start of $'", charT);
      }
      if(start->suffix().second != search_text.end())
      {
         BOOST_REGEX_TEST_ERROR("Incorrect position for end of $'", charT);
      }
      if(start->suffix().matched != (start->suffix().first != start->suffix().second))
      {
         BOOST_REGEX_TEST_ERROR("Incorrect position for matched member of $'", charT);
      }
      last_end = (*start)[0].second;
      ++start;
      ++copy;
      // move on the answer table to next set of answers;
      if(*answer_table != -2)
         while(*answer_table++ != -2){}
   }
   if(answer_table[0] >= 0)
   {
      // we should have had a match but didn't:
      BOOST_REGEX_TEST_ERROR("Expected match was not found.", charT);
   }
}

template<class charT, class traits>
void test_regex_token_iterator(boost::basic_regex<charT, traits>& r)
{
   typedef typename std::basic_string<charT>::const_iterator const_iterator;
   typedef boost::regex_token_iterator<const_iterator, charT, traits> test_iterator;
   const std::basic_string<charT>& search_text = test_info<charT>::search_text();
   boost::regex_constants::match_flag_type opts = test_info<charT>::match_options();
   const int* answer_table = test_info<charT>::answer_table();
   //
   // we start by testing sub-expression 0:
   //
   test_iterator start(search_text.begin(), search_text.end(), r, 0, opts), end;
   test_iterator copy(start);
   while(start != end)
   {
      if(start != copy)
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator != comparison.", charT);
      }
      if(!(start == copy))
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator == comparison.", charT);
      }
      test_sub_match(*start, search_text.begin(), answer_table, 0);
      ++start;
      ++copy;
      // move on the answer table to next set of answers;
      if(*answer_table != -2)
         while(*answer_table++ != -2){}
   }
   if(answer_table[0] >= 0)
   {
      // we should have had a match but didn't:
      BOOST_REGEX_TEST_ERROR("Expected match was not found.", charT);
   }
   //
   // and now field spitting:
   //
   test_iterator start2(search_text.begin(), search_text.end(), r, -1, opts), end2;
   test_iterator copy2(start2);
   int last_end2 = 0;
   answer_table = test_info<charT>::answer_table();
   while(start2 != end2)
   {
      if(start2 != copy2)
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator != comparison.", charT);
      }
      if(!(start2 == copy2))
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator == comparison.", charT);
      }
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4244)
#endif
      if(boost::re_detail::distance(search_text.begin(), start2->first) != last_end2)
      {
         BOOST_REGEX_TEST_ERROR(
            "Error in location of start of field split, found: " 
            << boost::re_detail::distance(search_text.begin(), start2->first)
            << ", expected: "
            << last_end2
            << ".", charT);
      }
      int expected_end = static_cast<int>(answer_table[0] < 0 ? search_text.size() : answer_table[0]);
      if(boost::re_detail::distance(search_text.begin(), start2->second) != expected_end)
      {
         BOOST_REGEX_TEST_ERROR(
            "Error in location of end2 of field split, found: "
            << boost::re_detail::distance(search_text.begin(), start2->second)
            << ", expected: "
            << expected_end
            << ".", charT);
      }
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
      last_end2 = answer_table[1];
      ++start2;
      ++copy2;
      // move on the answer table to next set of answers;
      if(*answer_table != -2)
         while(*answer_table++ != -2){}
   }
   if(answer_table[0] >= 0)
   {
      // we should have had a match but didn't:
      BOOST_REGEX_TEST_ERROR("Expected match was not found.", charT);
   }
#if !BOOST_WORKAROUND(BOOST_MSVC, < 1300)
   //
   // and now both field splitting and $0:
   //
   std::vector<int> subs;
   subs.push_back(-1);
   subs.push_back(0);
   start2 = test_iterator(search_text.begin(), search_text.end(), r, subs, opts);
   copy2 = start2;
   last_end2 = 0;
   answer_table = test_info<charT>::answer_table();
   while(start2 != end2)
   {
      if(start2 != copy2)
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator != comparison.", charT);
      }
      if(!(start2 == copy2))
      {
         BOOST_REGEX_TEST_ERROR("Failed iterator == comparison.", charT);
      }
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4244)
#endif
      if(boost::re_detail::distance(search_text.begin(), start2->first) != last_end2)
      {
         BOOST_REGEX_TEST_ERROR(
            "Error in location of start of field split, found: " 
            << boost::re_detail::distance(search_text.begin(), start2->first)
            << ", expected: "
            << last_end2
            << ".", charT);
      }
      int expected_end = static_cast<int>(answer_table[0] < 0 ? search_text.size() : answer_table[0]);
      if(boost::re_detail::distance(search_text.begin(), start2->second) != expected_end)
      {
         BOOST_REGEX_TEST_ERROR(
            "Error in location of end2 of field split, found: "
            << boost::re_detail::distance(search_text.begin(), start2->second)
            << ", expected: "
            << expected_end
            << ".", charT);
      }
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
      last_end2 = answer_table[1];
      ++start2;
      ++copy2;
      if((start2 == end2) && (answer_table[0] >= 0))
      {
         BOOST_REGEX_TEST_ERROR(
            "Expected $0 match not found", charT);
      }
      if(start2 != end2)
      {
         test_sub_match(*start2, search_text.begin(), answer_table, 0);
         ++start2;
         ++copy2;
      }
      // move on the answer table to next set of answers;
      if(*answer_table != -2)
         while(*answer_table++ != -2){}
   }
   if(answer_table[0] >= 0)
   {
      // we should have had a match but didn't:
      BOOST_REGEX_TEST_ERROR("Expected match was not found.", charT);
   }
#endif
}

template <class charT, class traits>
struct grep_test_predicate
{
   typedef typename std::basic_string<charT>::const_iterator test_iter;

   grep_test_predicate(test_iter b, const int* a)
      : m_base(b), m_table(a)
   {}
   bool operator()(const boost::match_results<test_iter>& what)
   {
      test_result(what, m_base, m_table);
      // move on the answer table to next set of answers;
      if(*m_table != -2)
         while(*m_table++ != -2){}
      return true;
   }
private:
   test_iter m_base;
   const int* m_table;
};

template<class charT, class traits>
void test_regex_grep(boost::basic_regex<charT, traits>& r)
{
   //typedef typename std::basic_string<charT>::const_iterator const_iterator;
   const std::basic_string<charT>& search_text = test_info<charT>::search_text();
   boost::regex_constants::match_flag_type opts = test_info<charT>::match_options();
   const int* answer_table = test_info<charT>::answer_table();
   grep_test_predicate<charT, traits> pred(search_text.begin(), answer_table);
   boost::regex_grep(pred, search_text.begin(), search_text.end(), r, opts);
}

template<class charT, class traits>
void test_regex_match(boost::basic_regex<charT, traits>& r)
{
   typedef typename std::basic_string<charT>::const_iterator const_iterator;
   const std::basic_string<charT>& search_text = test_info<charT>::search_text();
   boost::regex_constants::match_flag_type opts = test_info<charT>::match_options();
   const int* answer_table = test_info<charT>::answer_table();
   boost::match_results<const_iterator> what;
   if(answer_table[0] < 0)
   {
      if(boost::regex_match(search_text, r, opts))
      {
         BOOST_REGEX_TEST_ERROR("boost::regex_match found a match when it should not have done so.", charT);
      }
   }
   else
   {
      if((answer_table[0] > 0) && boost::regex_match(search_text, r, opts))
      {
         BOOST_REGEX_TEST_ERROR("boost::regex_match found a match when it should not have done so.", charT);
      }
      else if((answer_table[0] == 0) && (answer_table[1] == static_cast<int>(search_text.size())))
      {
         if(boost::regex_match(
            search_text.begin(),
            search_text.end(),
            what,
            r,
            opts))
         {
            test_result(what, search_text.begin(), answer_table);
         }
         else if(answer_table[0] >= 0)
         {
            // we should have had a match but didn't:
            BOOST_REGEX_TEST_ERROR("Expected match was not found.", charT);
         }
      }
   }
}

template<class charT, class traits>
void test(boost::basic_regex<charT, traits>& r, const test_regex_search_tag&)
{
   const std::basic_string<charT>& expression = test_info<charT>::expression();
   boost::regex_constants::syntax_option_type syntax_options = test_info<charT>::syntax_options();
   try{
      r.assign(expression, syntax_options);
      if(r.status())
      {
         BOOST_REGEX_TEST_ERROR("Expression did not compile when it should have done, error code = " << r.status(), charT);
      }
      if(expression != std::basic_string<charT>(r.begin(), r.end()))
      {
         BOOST_REGEX_TEST_ERROR("Stored expression string was incorrect", charT);
      }
      test_simple_search(r);
      test_regex_iterator(r);
      test_regex_token_iterator(r);
      test_regex_grep(r);
      test_regex_match(r);
      //
      // Verify sub-expression locations:
      //
      if((syntax_options & boost::regbase::save_subexpression_location) == 0)
      {
         bool have_except = false;
         try
         {
            r.subexpression(1);
         }
         catch(const std::out_of_range&)
         {
            have_except = true;
         }
         if(!have_except)
         {
            BOOST_REGEX_TEST_ERROR("Expected std::out_of_range error was not found.", charT);
         }
      }
      r.assign(expression, syntax_options | boost::regbase::save_subexpression_location);
      for(std::size_t i = 1; i < r.mark_count(); ++i)
      {
         std::pair<const charT*, const charT*> p = r.subexpression(i);
         if(*p.first != '(')
         {
            BOOST_REGEX_TEST_ERROR("Starting location of sub-expression " << i << " iterator was invalid.", charT);
         }
         if(*p.second != ')')
         {
            BOOST_REGEX_TEST_ERROR("Ending location of sub-expression " << i << " iterator was invalid.", charT);
         }
      }
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
