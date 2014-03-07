/*
 *
 * Copyright (c) 1998-2002
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE:        recursion_test.cpp
  *   VERSION:     see <boost/version.hpp>
  *   DESCRIPTION: Test for indefinite recursion and/or stack overrun.
  */

#include <string>
#include <boost/regex.hpp>
#include <boost/test/test_tools.hpp>

#ifdef BOOST_INTEL
#pragma warning(disable:1418 981 983 383)
#endif

int test_main( int , char* [] )
{
   // this regex will recurse twice for each whitespace character matched:
   boost::regex e("([[:space:]]|.)+");

   std::string bad_text(1024*1024*4, ' ');
   std::string good_text(200, ' ');

   boost::smatch what;

   //
   // Over and over: We want to make sure that after a stack error has
   // been triggered, that we can still conduct a good search and that
   // subsequent stack failures still do the right thing:
   //
   BOOST_CHECK(boost::regex_search(good_text, what, e));
   BOOST_CHECK_THROW(boost::regex_search(bad_text, what, e), std::runtime_error);
   BOOST_CHECK(boost::regex_search(good_text, what, e));
   BOOST_CHECK_THROW(boost::regex_search(bad_text, what, e), std::runtime_error);
   BOOST_CHECK(boost::regex_search(good_text, what, e));
   BOOST_CHECK_THROW(boost::regex_search(bad_text, what, e), std::runtime_error);
   BOOST_CHECK(boost::regex_search(good_text, what, e));
   BOOST_CHECK_THROW(boost::regex_search(bad_text, what, e), std::runtime_error);
   BOOST_CHECK(boost::regex_search(good_text, what, e));

   BOOST_CHECK(boost::regex_match(good_text, what, e));
   BOOST_CHECK_THROW(boost::regex_match(bad_text, what, e), std::runtime_error);
   BOOST_CHECK(boost::regex_match(good_text, what, e));
   BOOST_CHECK_THROW(boost::regex_match(bad_text, what, e), std::runtime_error);
   BOOST_CHECK(boost::regex_match(good_text, what, e));
   BOOST_CHECK_THROW(boost::regex_match(bad_text, what, e), std::runtime_error);
   BOOST_CHECK(boost::regex_match(good_text, what, e));
   BOOST_CHECK_THROW(boost::regex_match(bad_text, what, e), std::runtime_error);
   BOOST_CHECK(boost::regex_match(good_text, what, e));

   return 0;
}

#include <boost/test/included/test_exec_monitor.hpp>
