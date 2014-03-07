//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.

//  MACRO:         BOOST_NO_STD_MIN_MAX
//  TITLE:         std::min and std::max
//  DESCRIPTION:   The C++ standard library does not provide
//                 the (min)() and (max)() template functions that
//                 should be in <algorithm>.

#include <algorithm>

namespace boost_no_std_min_max{


int test()
{
   int i = 0;
   int j = 2;
   if((std::min)(i,j) != 0) return -1;
   if((std::max)(i,j) != 2) return -1;

   return 0;
}

}




