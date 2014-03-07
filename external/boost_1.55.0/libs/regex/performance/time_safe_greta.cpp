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
#if defined(BOOST_HAS_GRETA)

#include <cassert>
#include <boost/timer.hpp>
#include "regexpr2.h"

namespace gs{

double time_match(const std::string& re, const std::string& text, bool icase)
{
   regex::rpattern e(re, (icase ? regex::MULTILINE | regex::NORMALIZE | regex::NOCASE : regex::MULTILINE | regex::NORMALIZE), regex::MODE_SAFE);
   regex::match_results what;
   boost::timer tim;
   int iter = 1;
   int counter, repeats;
   double result = 0;
   double run;
   assert(e.match(text, what));
   do
   {
      tim.restart();
      for(counter = 0; counter < iter; ++counter)
      {
         e.match(text, what);
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
         e.match(text, what);
      }
      run = tim.elapsed();
      result = (std::min)(run, result);
   }
   return result / iter;
}

double time_find_all(const std::string& re, const std::string& text, bool icase)
{
   regex::rpattern e(re, (icase ? regex::MULTILINE | regex::NORMALIZE | regex::NOCASE : regex::MULTILINE | regex::NORMALIZE), regex::MODE_SAFE);
   regex::match_results what;
   boost::timer tim;
   int iter = 1;
   int counter, repeats;
   double result = 0;
   double run;
   do
   {
      bool r;
      tim.restart();
      for(counter = 0; counter < iter; ++counter)
      {
         e.match(text.begin(), text.end(), what);
         while(what.backref(0).matched)
         { 
            e.match(what.backref(0).end(), text.end(), what); 
         }
      }
      result = tim.elapsed();
      iter *= 2;
   }while(result < 0.5);
   iter /= 2;

   if(result > 10)
      return result / iter;

   // repeat test and report least value for consistency:
   for(repeats = 0; repeats < REPEAT_COUNT; ++repeats)
   {
      tim.restart();
      for(counter = 0; counter < iter; ++counter)
      {
         e.match(text.begin(), text.end(), what);
         while(what.backref(0).matched)
         { 
            e.match(what.backref(0).end(), text.end(), what); 
         }
      }
      run = tim.elapsed();
      result = (std::min)(run, result);
   }
   return result / iter;
}

}

#else

namespace gs{

double time_match(const std::string& re, const std::string& text, bool icase)
{
   return -1;
}

double time_find_all(const std::string& re, const std::string& text, bool icase)
{
   return -1;
}

}

#endif


