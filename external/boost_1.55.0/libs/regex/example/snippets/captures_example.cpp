/*
 *
 * Copyright (c) 2003-2004
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         captures_example.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Demonstrate the behaviour of captures.
  */

#include <boost/regex.hpp>
#include <iostream>


void print_captures(const std::string& regx, const std::string& text)
{
   boost::regex e(regx);
   boost::smatch what;
   std::cout << "Expression:  \"" << regx << "\"\n";
   std::cout << "Text:        \"" << text << "\"\n";
   if(boost::regex_match(text, what, e, boost::match_extra))
   {
      unsigned i, j;
      std::cout << "** Match found **\n   Sub-Expressions:\n";
      for(i = 0; i < what.size(); ++i)
         std::cout << "      $" << i << " = \"" << what[i] << "\"\n";
      std::cout << "   Captures:\n";
      for(i = 0; i < what.size(); ++i)
      {
         std::cout << "      $" << i << " = {";
         for(j = 0; j < what.captures(i).size(); ++j)
         {
            if(j)
               std::cout << ", ";
            else
               std::cout << " ";
            std::cout << "\"" << what.captures(i)[j] << "\"";
         }
         std::cout << " }\n";
      }
   }
   else
   {
      std::cout << "** No Match found **\n";
   }
}

int main(int , char* [])
{
   print_captures("(([[:lower:]]+)|([[:upper:]]+))+", "aBBcccDDDDDeeeeeeee");
   print_captures("a(b+|((c)*))+d", "abd");
   print_captures("(.*)bar|(.*)bah", "abcbar");
   print_captures("(.*)bar|(.*)bah", "abcbah");
   print_captures("^(?:(\\w+)|(?>\\W+))*$", "now is the time for all good men to come to the aid of the party");
   print_captures("^(?>(\\w+)\\W*)*$", "now is the time for all good men to come to the aid of the party");
   print_captures("^(\\w+)\\W+(?>(\\w+)\\W+)*(\\w+)$", "now is the time for all good men to come to the aid of the party");
   print_captures("^(\\w+)\\W+(?>(\\w+)\\W+(?:(\\w+)\\W+){0,2})*(\\w+)$", "now is the time for all good men to come to the aid of the party");
   return 0;
}

