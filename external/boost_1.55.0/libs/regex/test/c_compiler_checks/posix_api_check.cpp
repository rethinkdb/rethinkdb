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
  *   FILE         posix_api_compiler_check.c
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Verify that POSIX API calls compile: note this is a compile
  *                time check only.
  */

#include <stdio.h>
#include <string.h>
#include <boost/assert.hpp>
#include <boost/regex.h>
#include <boost/detail/lightweight_test.hpp>

const char* expression = "^";
const char* text = "\n      ";
regmatch_t matches[1];
int flags = REG_EXTENDED | REG_BASIC | REG_NOSPEC | REG_ICASE | REG_NOSUB |
            REG_NEWLINE | REG_PEND | REG_NOCOLLATE | REG_ESCAPE_IN_LISTS |
            REG_NEWLINE_ALT | REG_PERL | REG_AWK | REG_GREP | REG_EGREP;


int main()
{
   regex_tA re;
   unsigned int result;
   result = regcompA(&re, expression, REG_AWK);
   if(result > REG_NOERROR)
   {
      char buf[256];
      regerrorA(result, &re, buf, sizeof(buf));
      printf("%s", buf);
      return result;
   }
   BOOST_TEST(re.re_nsub == 0);
   matches[0].rm_so = 0;
   matches[0].rm_eo = strlen(text);
   result = regexecA(&re, text, 1, matches, REG_NOTBOL | REG_NOTEOL | REG_STARTEND);
   if(result > REG_NOERROR)
   {
      char buf[256];
      regerrorA(result, &re, buf, sizeof(buf));
      printf("%s", buf);
      regfreeA(&re);
      return result;
   }
   BOOST_TEST(matches[0].rm_so == matches[0].rm_eo);
   regfreeA(&re);
   printf("no errors found\n");
   return boost::report_errors();
}



