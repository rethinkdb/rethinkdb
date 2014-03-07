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

#include <cassert>
#include <cfloat>
#include "regex_comparison.hpp"
#ifdef BOOST_HAS_POSIX
#include <boost/timer.hpp>
#include "regex.h"

namespace posix{

double time_match(const std::string& re, const std::string& text, bool icase)
{
   regex_t e;
   regmatch_t what[20];
   boost::timer tim;
   int iter = 1;
   int counter, repeats;
   double result = 0;
   double run;
   if(0 != ::regcomp(&e, re.c_str(), (icase ? REG_ICASE | REG_EXTENDED : REG_EXTENDED)))
      return -1;
   do
   {
      tim.restart();
      for(counter = 0; counter < iter; ++counter)
      {
         regexec(&e, text.c_str(), e.re_nsub, what, 0);
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
         regexec(&e, text.c_str(), e.re_nsub, what, 0);
      }
      run = tim.elapsed();
      result = (std::min)(run, result);
   }
   regfree(&e);
   return result / iter;
}

double time_find_all(const std::string& re, const std::string& text, bool icase)
{
   regex_t e;
   regmatch_t what[20];
   memset(what, 0, sizeof(what));
   boost::timer tim;
   int iter = 1;
   int counter, repeats;
   double result = 0;
   double run;
   int exec_result;
   int matches;
   if(0 != regcomp(&e, re.c_str(), (icase ? REG_ICASE | REG_EXTENDED : REG_EXTENDED)))
      return -1;
   do
   {
      tim.restart();
      for(counter = 0; counter < iter; ++counter)
      {
         what[0].rm_so = 0;
         what[0].rm_eo = text.size();
         matches = 0;
         exec_result = regexec(&e, text.c_str(), 20, what, REG_STARTEND);
         while(exec_result == 0)
         { 
            ++matches;
            what[0].rm_so = what[0].rm_eo;
            what[0].rm_eo = text.size();
            exec_result = regexec(&e, text.c_str(), 20, what, REG_STARTEND);
         }
      }
      result = tim.elapsed();
      iter *= 2;
   }while(result < 0.5);
   iter /= 2;

   if(result >10)
      return result / iter;

   result = DBL_MAX;

   // repeat test and report least value for consistency:
   for(repeats = 0; repeats < REPEAT_COUNT; ++repeats)
   {
      tim.restart();
      for(counter = 0; counter < iter; ++counter)
      {
         what[0].rm_so = 0;
         what[0].rm_eo = text.size();
         matches = 0;
         exec_result = regexec(&e, text.c_str(), 20, what, REG_STARTEND);
         while(exec_result == 0)
         { 
            ++matches;
            what[0].rm_so = what[0].rm_eo;
            what[0].rm_eo = text.size();
            exec_result = regexec(&e, text.c_str(), 20, what, REG_STARTEND);
         }
      }
      run = tim.elapsed();
      result = (std::min)(run, result);
   }
   return result / iter;
}

}
#else

namespace posix{

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
