//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.

//  MACRO:         BOOST_NO_STRINGSTREAM
//  TITLE:         <sstream>
//  DESCRIPTION:   The C++ implementation does not provide the <sstream> header.

#include <sstream>
#include <string>

namespace boost_no_stringstream{

int test()
{
   std::stringstream ss;
   ss << "abc";
   std::string s = ss.str();
   return (s != "abc");
}

}




