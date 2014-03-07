//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.

//  MACRO:         BOOST_NO_STD_LOCALE
//  TITLE:         std::locale
//  DESCRIPTION:   The standard library lacks std::locale.

#include <locale>

namespace boost_no_std_locale{

int test()
{
   std::locale l1;
   //
   // ideally we would construct a locale from a facet,
   // but that requires template member functions which 
   // may not be available, instead just check that we can
   // construct a pointer to a facet:
   //
   const std::ctype<char>* pct = 0;
   (void) &l1;
   (void) &pct;
   return 0;
}

}





