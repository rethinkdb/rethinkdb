//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_STLP_USE_FACET
//  TITLE:         STLport version of use_facet
//  DESCRIPTION:   The standard library lacks a conforming std::use_facet,
//                 but has a workaound class-version that does the job.
//                 This is primarily for the STLport std lib.

#include <locale>


namespace boost_has_stlp_use_facet{

int test()
{
   std::locale l;
   const std::ctype<char>& ct = *std::_Use_facet<std::ctype<char> >(l);
   return 0;
}

}





