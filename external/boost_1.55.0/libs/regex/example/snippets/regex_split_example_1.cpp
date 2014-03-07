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
  *   FILE         regex_split_example_1.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_split example: split a string into tokens.
  */


#include <list>
#include <boost/regex.hpp>


unsigned tokenise(std::list<std::string>& l, std::string& s)
{
   return boost::regex_split(std::back_inserter(l), s);
}

#include <iostream>
using namespace std;


#if defined(BOOST_MSVC) || (defined(__BORLANDC__) && (__BORLANDC__ == 0x550))
//
// problem with std::getline under MSVC6sp3
istream& getline(istream& is, std::string& s)
{
   s.erase();
   char c = static_cast<char>(is.get());
   while(c != '\n')
   {
      s.append(1, c);
      c = static_cast<char>(is.get());
   }
   return is;
}
#endif


int main(int argc, const char*[])
{
   string s;
   list<string> l;
   do{
      if(argc == 1)
      {
         cout << "Enter text to split (or \"quit\" to exit): ";
         getline(cin, s);
         if(s == "quit") break;
      }
      else
         s = "This is a string of tokens";
      unsigned result = tokenise(l, s);
      cout << result << " tokens found" << endl;
      cout << "The remaining text is: \"" << s << "\"" << endl;
      while(l.size())
      {
         s = *(l.begin());
         l.pop_front();
         cout << s << endl;
      }
   }while(argc == 1);
   return 0;
}


