//  (C) Copyright John Maddock 2003. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.

//
// before we do anything else, we need to mess with the compilers ABI:
//
#include <boost/config.hpp>
#ifdef BOOST_MSVC
#pragma pack(1)
#elif defined(__BORLANDC__)
#pragma option -Ve- -Vx- -a1 -b-
#endif
#include <stdio.h>
#include "abi_test.hpp"


int main()
{
   abi_test t;
   if((t.inline_one() != t.virtual_one()) || (t.inline_two() != t.virtual_two()))
   {
      fwrite("Failed ABI test", 1, 15, stdout);
      return -1;
   }
   return 0;
}

