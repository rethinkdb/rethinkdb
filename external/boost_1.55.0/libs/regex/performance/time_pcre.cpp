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
#ifdef BOOST_HAS_PCRE
#include "pcre.h"
#include <boost/timer.hpp>

namespace pcr{

double time_match(const std::string& re, const std::string& text, bool icase)
{
   pcre *ppcre;
   const char *error;
   int erroffset;
  
   int what[50];

   boost::timer tim;
   int iter = 1;
   int counter, repeats;
   double result = 0;
   double run;

   if(0 == (ppcre = pcre_compile(re.c_str(), (icase ? PCRE_CASELESS | PCRE_ANCHORED | PCRE_DOTALL | PCRE_MULTILINE : PCRE_ANCHORED | PCRE_DOTALL | PCRE_MULTILINE),
      &error, &erroffset, NULL)))
   {
      free(ppcre);
      return -1;
   }

   pcre_extra *pe;
   pe = pcre_study(ppcre, 0, &error);
   if(error)
   {
      free(ppcre);
      free(pe);
      return -1;
   }

   do
   {
      tim.restart();
      for(counter = 0; counter < iter; ++counter)
      {
         erroffset = pcre_exec(ppcre, pe, text.c_str(), text.size(), 0, 0, what, sizeof(what)/sizeof(int));
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
         erroffset = pcre_exec(ppcre, pe, text.c_str(), text.size(), 0, 0, what, sizeof(what)/sizeof(int));
      }
      run = tim.elapsed();
      result = (std::min)(run, result);
   }
   free(ppcre);
   free(pe);
   return result / iter;
}

double time_find_all(const std::string& re, const std::string& text, bool icase)
{
   pcre *ppcre;
   const char *error;
   int erroffset;
  
   int what[50];

   boost::timer tim;
   int iter = 1;
   int counter, repeats;
   double result = 0;
   double run;
   int exec_result;
   int matches;

   if(0 == (ppcre = pcre_compile(re.c_str(), (icase ? PCRE_CASELESS | PCRE_DOTALL | PCRE_MULTILINE : PCRE_DOTALL | PCRE_MULTILINE), &error, &erroffset, NULL)))
   {
      free(ppcre);
      return -1;
   }

   pcre_extra *pe;
   pe = pcre_study(ppcre, 0, &error);
   if(error)
   {
      free(ppcre);
      free(pe);
      return -1;
   }

   do
   {
      int startoff;
      tim.restart();
      for(counter = 0; counter < iter; ++counter)
      {
         matches = 0;
         startoff = 0;
         exec_result = pcre_exec(ppcre, pe, text.c_str(), text.size(), startoff, 0, what, sizeof(what)/sizeof(int));
         while(exec_result >= 0)
         { 
            ++matches;
            startoff = what[1];
            exec_result = pcre_exec(ppcre, pe, text.c_str(), text.size(), startoff, 0, what, sizeof(what)/sizeof(int));
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
      int startoff;
      matches = 0;
      tim.restart();
      for(counter = 0; counter < iter; ++counter)
      {
         matches = 0;
         startoff = 0;
         exec_result = pcre_exec(ppcre, pe, text.c_str(), text.size(), startoff, 0, what, sizeof(what)/sizeof(int));
         while(exec_result >= 0)
         { 
            ++matches;
            startoff = what[1];
            exec_result = pcre_exec(ppcre, pe, text.c_str(), text.size(), startoff, 0, what, sizeof(what)/sizeof(int));
         }
      }
      run = tim.elapsed();
      result = (std::min)(run, result);
   }
   return result / iter;
}

}
#else

namespace pcr{

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
