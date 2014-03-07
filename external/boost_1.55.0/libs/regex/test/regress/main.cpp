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
  *   FILE         main.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: entry point for test program.
  */

#include "test.hpp"
#include "test_locale.hpp"
#include <stdarg.h>

#ifdef BOOST_HAS_ICU
#include <unicode/uloc.h>
#endif

#ifdef TEST_THREADS
#include <list>
#include <boost/thread.hpp>
#include <boost/thread/tss.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/array.hpp>

int* get_array_data();

#endif

int error_count = 0;

#ifndef TEST_THREADS
#define RUN_TESTS(name) \
   std::cout << "Running test case \"" #name "\".\n";\
   name();
#else
#define RUN_TESTS(name) \
   name();
#endif


void run_tests()
{
   RUN_TESTS(basic_tests);
   RUN_TESTS(test_simple_repeats);
   RUN_TESTS(test_alt);
   RUN_TESTS(test_sets);
   RUN_TESTS(test_sets2);
   RUN_TESTS(test_anchors);
   RUN_TESTS(test_backrefs);
   RUN_TESTS(test_character_escapes);
   RUN_TESTS(test_assertion_escapes);
   RUN_TESTS(test_tricky_cases);
   RUN_TESTS(test_grep);
   RUN_TESTS(test_replace);
   RUN_TESTS(test_non_greedy_repeats);
   RUN_TESTS(test_non_marking_paren);
   RUN_TESTS(test_partial_match);
   RUN_TESTS(test_forward_lookahead_asserts);
   RUN_TESTS(test_fast_repeats);
   RUN_TESTS(test_fast_repeats2);
   RUN_TESTS(test_independent_subs);
   RUN_TESTS(test_nosubs);
   RUN_TESTS(test_conditionals);
   RUN_TESTS(test_options);
   RUN_TESTS(test_options2);
#ifndef TEST_THREADS
   RUN_TESTS(test_en_locale);
#endif
   RUN_TESTS(test_emacs);
   RUN_TESTS(test_operators);
   RUN_TESTS(test_overloads);
   RUN_TESTS(test_unicode);
   RUN_TESTS(test_pocessive_repeats);
   RUN_TESTS(test_mark_resets);
   RUN_TESTS(test_recursion);
}

int cpp_main(int /*argc*/, char * /*argv*/[])
{
#ifdef BOOST_HAS_ICU
   //
   // We need to set the default locale used by ICU, 
   // otherwise some of our tests using equivalence classes fail.
   //
   UErrorCode err = U_ZERO_ERROR;
   uloc_setDefault("en", &err);
   if(err != U_ZERO_ERROR)
   {
      std::cerr << "Unable to set the default ICU locale to \"en\"." << std::endl;
      return -1;
   }
#endif
#ifdef TEST_THREADS
   try{
      get_array_data();  // initialises data.
   }
   catch(const std::exception& e)
   {
      std::cerr << "TSS Initialisation failed with message: " << e.what() << std::endl;
      return -1;
   }

   std::list<boost::shared_ptr<boost::thread> > threads;
   for(int i = 0; i < 5; ++i)
   {
      try{
         threads.push_back(boost::shared_ptr<boost::thread>(new boost::thread(&run_tests)));
      }
      catch(const std::exception& e)
      {
         std::cerr << "<note>Thread creation failed with message: " << e.what() << "</note>" << std::endl;
      }
   }
   std::list<boost::shared_ptr<boost::thread> >::const_iterator a(threads.begin()), b(threads.end());
   while(a != b)
   {
      (*a)->join();
      ++a;
   }
#else
   run_tests();
#endif
   return error_count;
}

#ifdef TEST_THREADS

int* get_array_data()
{
   static boost::thread_specific_ptr<boost::array<int, 200> > tp;

   if(tp.get() == 0)
      tp.reset(new boost::array<int, 200>);

   return tp.get()->data();
}

#endif

const int* make_array(int first, ...)
{
   //
   // this function takes a variable number of arguments
   // and packs them into an array that we can pass through
   // our testing macros (ideally we would use an array literal
   // but these can't apparently be used as macro arguments).
   //
#ifdef TEST_THREADS
   int* data = get_array_data();
#else
   static int data[200];
#endif
   va_list ap;
   va_start(ap, first);
   //
   // keep packing args, until we get two successive -2 values:
   //
   int terminator_count;
   int next_position = 1;
   data[0] = first;
   if(first == -2)
      terminator_count = 1;
   else
      terminator_count = 0;
   while(terminator_count < 2)
   {
      data[next_position] = va_arg(ap, int);
      if(data[next_position] == -2)
         ++terminator_count;
      else
         terminator_count = 0;
      ++next_position;
   }
   va_end(ap);
   return data;
}

#ifdef BOOST_NO_EXCEPTIONS

namespace boost{

void throw_exception(std::exception const & e)
{
   std::abort();
}

}

#endif

void test(const char& c, const test_regex_replace_tag& tag)
{
   do_test(c, tag);
}
void test(const char& c, const test_regex_search_tag& tag)
{
   do_test(c, tag);
}
void test(const char& c, const test_invalid_regex_tag& tag)
{
   do_test(c, tag);
}

#ifndef BOOST_NO_WREGEX
void test(const wchar_t& c, const test_regex_replace_tag& tag)
{
   do_test(c, tag);
}
void test(const wchar_t& c, const test_regex_search_tag& tag)
{
   do_test(c, tag);
}
void test(const wchar_t& c, const test_invalid_regex_tag& tag)
{
   do_test(c, tag);
}
#endif

#ifdef BOOST_NO_EXCETIONS
namespace boost{

void throw_exception( std::exception const & e )
{
   std::cerr << e.what() << std::endl;
   std::exit(1);
}

}
#endif

#include <boost/test/included/prg_exec_monitor.hpp>
