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
  *   FILE         mfc_example.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: examples of using Boost.Regex with MFC and ATL string types.
  */

#ifdef TEST_MFC

#include <boost/regex/mfc.hpp>
#include <cstringt.h>
#include <atlstr.h>
#include <assert.h>
#include <tchar.h>
#include <iostream>

#ifdef _UNICODE
#define cout wcout
#endif

//
// Find out if *password* meets our password requirements,
// as defined by the regular expression *requirements*.
//
bool is_valid_password(const CString& password, const CString& requirements)
{
   return boost::regex_match(password, boost::make_regex(requirements));
}

//
// Extract filename part of a path from a CString and return the result
// as another CString:
//
CString get_filename(const CString& path)
{
   boost::tregex r(__T("(?:\\A|.*\\\\)([^\\\\]+)"));
   boost::tmatch what;
   if(boost::regex_match(path, what, r))
   {
      // extract $1 as a CString:
      return CString(what[1].first, what.length(1));
   }
   else
   {
      throw std::runtime_error("Invalid pathname");
   }
}

CString extract_postcode(const CString& address)
{
   // searches throw address for a UK postcode and returns the result,
   // the expression used is by Phil A. on www.regxlib.com:
   boost::tregex r(__T("^(([A-Z]{1,2}[0-9]{1,2})|([A-Z]{1,2}[0-9][A-Z]))\\s?([0-9][A-Z]{2})$"));
   boost::tmatch what;
   if(boost::regex_search(address, what, r))
   {
      // extract $0 as a CString:
      return CString(what[0].first, what.length());
   }
   else
   {
      throw std::runtime_error("No postcode found");
   }
}

void enumerate_links(const CString& html)
{
   // enumerate and print all the <a> links in some HTML text,
   // the expression used is by Andew Lee on www.regxlib.com:
   boost::tregex r(__T("href=[\"\']((http:\\/\\/|\\.\\/|\\/)?\\w+(\\.\\w+)*(\\/\\w+(\\.\\w+)?)*(\\/|\\?\\w*=\\w*(&\\w*=\\w*)*)?)[\"\']"));
   boost::tregex_iterator i(boost::make_regex_iterator(html, r)), j;
   while(i != j)
   {
      std::cout << (*i)[1] << std::endl;
      ++i;
   }
}

void enumerate_links2(const CString& html)
{
   // enumerate and print all the <a> links in some HTML text,
   // the expression used is by Andew Lee on www.regxlib.com:
   boost::tregex r(__T("href=[\"\']((http:\\/\\/|\\.\\/|\\/)?\\w+(\\.\\w+)*(\\/\\w+(\\.\\w+)?)*(\\/|\\?\\w*=\\w*(&\\w*=\\w*)*)?)[\"\']"));
   boost::tregex_token_iterator i(boost::make_regex_token_iterator(html, r, 1)), j;
   while(i != j)
   {
      std::cout << *i << std::endl;
      ++i;
   }
}

//
// Take a credit card number as a string of digits, 
// and reformat it as a human readable string with "-"
// separating each group of four digits:
//
const boost::tregex e(__T("\\A(\\d{3,4})[- ]?(\\d{4})[- ]?(\\d{4})[- ]?(\\d{4})\\z"));
const CString human_format = __T("$1-$2-$3-$4");

CString human_readable_card_number(const CString& s)
{
   return boost::regex_replace(s, e, human_format);
}


int main()
{
   // password checks using regex_match:
   CString pwd = "abcDEF---";
   CString pwd_check = "(?=.*[[:lower:]])(?=.*[[:upper:]])(?=.*[[:punct:]]).{6,}";
   bool b = is_valid_password(pwd, pwd_check);
   assert(b);
   pwd = "abcD-";
   b = is_valid_password(pwd, pwd_check);
   assert(!b);

   // filename extraction with regex_match:
   CString file = "abc.hpp";
   file = get_filename(file);
   assert(file == "abc.hpp");
   file = "c:\\a\\b\\c\\d.h";
   file = get_filename(file);
   assert(file == "d.h");

   // postcode extraction with regex_search:
   CString address = "Joe Bloke, 001 Somestreet, Somewhere,\nPL2 8AB";
   CString postcode = extract_postcode(address);
   assert(postcode = "PL2 8NV");

   // html link extraction with regex_iterator:
   CString text = "<dt><a href=\"syntax_perl.html\">Perl Regular Expressions</a></dt><dt><a href=\"syntax_extended.html\">POSIX-Extended Regular Expressions</a></dt><dt><a href=\"syntax_basic.html\">POSIX-Basic Regular Expressions</a></dt>";
   enumerate_links(text);
   enumerate_links2(text);

   CString credit_card_number = "1234567887654321";
   credit_card_number = human_readable_card_number(credit_card_number);
   assert(credit_card_number == "1234-5678-8765-4321");
   return 0;
}

#else

#include <iostream>

int main()
{
   std::cout << "<NOTE>MFC support not enabled, feature unavailable</NOTE>";
   return 0;
}

#endif
