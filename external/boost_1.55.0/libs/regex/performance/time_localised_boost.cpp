/*
 *
 * Copyright (c) 2002
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include "regex_comparison.hpp"
#include <boost/timer.hpp>
#include <boost/regex.hpp>

namespace bl{

double time_match(const std::string& re, const std::string& text, bool icase)
{
   boost::basic_regex<char, boost::cpp_regex_traits<char> > e(re, (icase ? boost::regex::perl | boost::regex::icase : boost::regex::perl));
   boost::smatch what;
   boost::timer tim;
   int iter = 1;
   int counter, repeats;
   double result = 0;
   double run;
   do
   {
      tim.restart();
      for(counter = 0; counter < iter; ++counter)
      {
         boost::regex_match(text, what, e);
      }
      result = tim.elapsed();
      iter *= 2;
   }while(result < 0.5);
   iter /= 2;

   // repeat test and report least value for consistency:
   for(repeats = 0; repeats < REPEAT_COUNT; ++repeats)
   {
      tim.restart();
      for(counter = 0; counter < iter; ++counter)
      {
         boost::regex_match(text, what, e);
      }
      run = tim.elapsed();
      result = (std::min)(run, result);
   }
   return result / iter;
}

bool dummy_grep_proc(const boost::smatch&)
{ return true; }

struct noop
{
    void operator()( boost::smatch const & ) const
    {
    }
};

double time_find_all(const std::string& re, const std::string& text, bool icase)
{
   boost::basic_regex<char, boost::cpp_regex_traits<char> > e(re, (icase ? boost::regex::perl | boost::regex::icase : boost::regex::perl));
   boost::smatch what;
   boost::timer tim;
   int iter = 1;
   int counter, repeats;
   double result = 0;
   double run;
   do
   {
      tim.restart();
      for(counter = 0; counter < iter; ++counter)
      {
         boost::regex_iterator<
            std::string::const_iterator, 
            char, 
            boost::cpp_regex_traits<char> > begin( text.begin(), text.end(), e ), end;
         std::for_each( begin, end, noop() );
         //boost::regex_grep(&dummy_grep_proc, text, e);
      }
      result = tim.elapsed();
      iter *= 2;
   }while(result < 0.5);
   iter /= 2;

   if(result >10)
      return result / iter;

   // repeat test and report least value for consistency:
   for(repeats = 0; repeats < REPEAT_COUNT; ++repeats)
   {
      tim.restart();
      for(counter = 0; counter < iter; ++counter)
      {
         boost::regex_grep(&dummy_grep_proc, text, e);
      }
      run = tim.elapsed();
      result = (std::min)(run, result);
   }
   return result / iter;
}

}

