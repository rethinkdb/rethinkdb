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
  *   FILE         regex_split_example_2.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_split example: spit out linked URL's.
  */


#include <list>
#include <fstream>
#include <iostream>
#include <iterator>
#include <boost/regex.hpp>

boost::regex e("<\\s*A\\s+[^>]*href\\s*=\\s*\"([^\"]*)\"",
               boost::regex::normal | boost::regbase::icase);

void load_file(std::string& s, std::istream& is)
{
   s.erase();
   if(is.bad()) return;
   //
   // attempt to grow string buffer to match file size,
   // this doesn't always work...
   s.reserve(static_cast<std::string::size_type>(is.rdbuf()->in_avail()));
   char c;
   while(is.get(c))
   {
      // use logarithmic growth stategy, in case
      // in_avail (above) returned zero:
      if(s.capacity() == s.size())
         s.reserve(s.capacity() * 3);
      s.append(1, c);
   }
}

int main(int argc, char** argv)
{
   std::string s;
   std::list<std::string> l;
   int i;
   for(i = 1; i < argc; ++i)
   {
      std::cout << "Findings URL's in " << argv[i] << ":" << std::endl;
      s.erase();
      std::ifstream is(argv[i]);
      load_file(s, is);
      is.close();
      boost::regex_split(std::back_inserter(l), s, e);
      while(l.size())
      {
         s = *(l.begin());
         l.pop_front();
         std::cout << s << std::endl;
      }
   }
   //
   // alternative method:
   // split one match at a time and output direct to
   // cout via ostream_iterator<std::string>....
   //
   for(i = 1; i < argc; ++i)
   {
      std::cout << "Findings URL's in " << argv[i] << ":" << std::endl;
      s.erase();
      std::ifstream is(argv[i]);
      load_file(s, is);
      is.close();
      while(boost::regex_split(std::ostream_iterator<std::string>(std::cout), s, e, boost::match_default, 1)) std::cout << std::endl;
   }

   return 0;
}


