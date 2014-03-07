//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_MACRO_USE_FACET
//  TITLE:         macro version of use_facet: _USE
//  DESCRIPTION:   The standard library lacks a conforming std::use_facet,
//                 but has a macro _USE(loc, Type) that does the job.
//                 This is primarily for the Dinkumware std lib.

#include <locale>

#ifndef _USE
#error "macro _USE not defined"
#endif

namespace boost_has_macro_use_facet{

int test()
{
   std::locale l;
   const std::ctype<char>& ct = std::_USE(l, std::ctype<char>);
   return 0;
}

}






