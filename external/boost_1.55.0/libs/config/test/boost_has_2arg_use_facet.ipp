//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_TWO_ARG_USE_FACET
//  TITLE:         two argument version of use_facet
//  DESCRIPTION:   The standard library lacks a conforming std::use_facet,
//                 but has a two argument version that does the job.
//                 This is primarily for the Rogue Wave std lib.

#include <locale>


namespace boost_has_two_arg_use_facet{

int test()
{
   std::locale l;
   const std::ctype<char>& ct = std::use_facet(l, (std::ctype<char>*)0);
   return 0;
}

}






