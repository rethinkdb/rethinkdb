/*
 *
 * Copyright (c) 2003
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_iterator_example_2.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_iterator example 2: searches a cpp file for class definitions,
  *                using global data.
  */

#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <boost/regex.hpp>

using namespace std;

// purpose:
// takes the contents of a file in the form of a string
// and searches for all the C++ class definitions, storing
// their locations in a map of strings/int's

typedef std::map<std::string, std::string::difference_type, std::less<std::string> > map_type;

const char* re = 
   // possibly leading whitespace:   
   "^[[:space:]]*" 
   // possible template declaration:
   "(template[[:space:]]*<[^;:{]+>[[:space:]]*)?"
   // class or struct:
   "(class|struct)[[:space:]]*" 
   // leading declspec macros etc:
   "("
      "\\<\\w+\\>"
      "("
         "[[:blank:]]*\\([^)]*\\)"
      ")?"
      "[[:space:]]*"
   ")*" 
   // the class name
   "(\\<\\w*\\>)[[:space:]]*" 
   // template specialisation parameters
   "(<[^;:{]+>)?[[:space:]]*"
   // terminate in { or :
   "(\\{|:[^;\\{()]*\\{)";


boost::regex expression(re);
map_type class_index;

bool regex_callback(const boost::match_results<std::string::const_iterator>& what)
{
   // what[0] contains the whole string
   // what[5] contains the class name.
   // what[6] contains the template specialisation if any.
   // add class name and position to map:
   class_index[what[5].str() + what[6].str()] = what.position(5);
   return true;
}

void load_file(std::string& s, std::istream& is)
{
   s.erase();
   if(is.bad()) return;
   s.reserve(static_cast<std::string::size_type>(is.rdbuf()->in_avail()));
   char c;
   while(is.get(c))
   {
      if(s.capacity() == s.size())
         s.reserve(s.capacity() * 3);
      s.append(1, c);
   }
}

int main(int argc, const char** argv)
{
   std::string text;
   for(int i = 1; i < argc; ++i)
   {
      cout << "Processing file " << argv[i] << endl;
      std::ifstream fs(argv[i]);
      load_file(text, fs);
      fs.close();
      // construct our iterators:
      boost::sregex_iterator m1(text.begin(), text.end(), expression);
      boost::sregex_iterator m2;
      std::for_each(m1, m2, &regex_callback);
      // copy results:
      cout << class_index.size() << " matches found" << endl;
      map_type::iterator c, d;
      c = class_index.begin();
      d = class_index.end();
      while(c != d)
      {
         cout << "class \"" << (*c).first << "\" found at index: " << (*c).second << endl;
         ++c;
      }
      class_index.erase(class_index.begin(), class_index.end());
   }
   return 0;
}



