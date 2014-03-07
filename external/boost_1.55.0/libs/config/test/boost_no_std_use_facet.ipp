//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.

//  MACRO:         BOOST_NO_STD_USE_FACET
//  TITLE:         std::use_facet
//  DESCRIPTION:   The standard library lacks a conforming std::use_facet.

#include <locale>

namespace boost_no_std_use_facet{

int test()
{
   std::locale l;
   const std::ctype<char>& ct = std::use_facet<std::ctype<char> >(l);
   (void)ct;
   return 0;
}

}





