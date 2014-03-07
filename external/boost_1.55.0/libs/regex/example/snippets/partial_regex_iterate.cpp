/*
 *
 * Copyright (c) 1998-2007
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         partial_regex_iterate.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Search example using partial matches.
  */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <boost/regex.hpp>

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std{ using ::memmove; }
#endif

// match some kind of html tag:
boost::regex e("<[^>]*>");
// count how many:
unsigned int tags = 0;

void search(std::istream& is)
{
   // buffer we'll be searching in:
   char buf[4096];
   // saved position of end of partial match:
   const char* next_pos = buf + sizeof(buf);
   // flag to indicate whether there is more input to come:
   bool have_more = true;

   while(have_more)
   {
      // how much do we copy forward from last try:
      std::ptrdiff_t leftover = (buf + sizeof(buf)) - next_pos;
      // and how much is left to fill:
      std::ptrdiff_t size = next_pos - buf;
      // copy forward whatever we have left:
      std::memmove(buf, next_pos, leftover);
      // fill the rest from the stream:
      is.read(buf + leftover, size);
      std::streamsize read = is.gcount();
      // check to see if we've run out of text:
      have_more = read == size;
      // reset next_pos:
      next_pos = buf + sizeof(buf);
      // and then iterate:
      boost::cregex_iterator a(
         buf, 
         buf + read + leftover, 
         e, 
         boost::match_default | boost::match_partial); 
      boost::cregex_iterator b;

      while(a != b)
      {
         if((*a)[0].matched == false)
         {
            // Partial match, save position and break:
            next_pos = (*a)[0].first;
            break;
         }
         else
         {
            // full match:
            ++tags;
         }
         
         // move to next match:
         ++a;
      }
   }
}

int main(int argc, char* argv[])
{
   if(argc > 1)
   {
      for(int i = 1; i < argc; ++i)
      {
         std::ifstream fs(argv[i]);
         if(fs.bad()) continue;
         search(fs);
         fs.close();
      }
   }
   else
   {
      std::string one("<META NAME=\"keywords\" CONTENT=\"regex++, regular expressions, regular expression library, C++\">");
      std::string what;
      while(what.size() < 10000)
      {
         what.append(one);
         what.append(13, ' ');
      }
      std::stringstream ss;
      ss.str(what);
      search(ss);
   }
   std::cout << "total tag count was " << tags << std::endl;
   return 0;
}




