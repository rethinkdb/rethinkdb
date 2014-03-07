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
  *   FILE         wide_posix_api_compiler_check.c
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Verify that POSIX API calls compile: note this is a compile
  *                time check only.
  */

#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <boost/regex.h>
#include <wchar.h>

#ifndef BOOST_NO_WREGEX

const wchar_t* expression = L"^";
const wchar_t* text = L"\n      ";
regmatch_t matches[1];
int flags = REG_EXTENDED | REG_BASIC | REG_NOSPEC | REG_ICASE | REG_NOSUB |
            REG_NEWLINE | REG_PEND | REG_NOCOLLATE | REG_ESCAPE_IN_LISTS |
            REG_NEWLINE_ALT | REG_PERL | REG_AWK | REG_GREP | REG_EGREP;


int main()
{
   regex_t re;
   unsigned result;
   result = regcomp(&re, expression, REG_AWK);
   if(result > REG_NOERROR)
   {
      wchar_t buf[256];
      regerror(result, &re, buf, sizeof(buf));
      char nbuf[256];
      for(int i = 0; i < 256; ++i)
         nbuf[i] = static_cast<char>(buf[i]);
      printf("%s", nbuf);
      return result;
   }
   if(re.re_nsub != 0)
   {
      regfree(&re);
      exit(-1);
   }
   matches[0].rm_so = 0;
   matches[0].rm_eo = wcslen(text);
   result = regexec(&re, text, 1, matches, REG_NOTBOL | REG_NOTEOL | REG_STARTEND);
   if(result > REG_NOERROR)
   {
      wchar_t buf[256];
      regerror(result, &re, buf, sizeof(buf));
      char nbuf[256];
      for(int i = 0; i < 256; ++i)
         nbuf[i] = static_cast<char>(buf[i]);
      printf("%s", nbuf);
      regfree(&re);
      return result;
   }
   if((matches[0].rm_so != matches[0].rm_eo) || (matches[0].rm_eo != 1))
   {
      regfree(&re);
      exit(-1);
   }
   regfree(&re);
   printf("%s", "no errors found\n");
   return 0;
}

#else

#include <iostream> 

int main()
{
   std::cout <<
   "\n<note>\n"
   "This platform does not provide the needed wide character support for this test.\n"
   "</note>\n";
   return 0;
}
#endif




