//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.

//  MACRO:         BOOST_NO_STD_DISTANCE
//  TITLE:         std::distance
//  DESCRIPTION:   The platform does not have a conforming version of std::distance.

#include <algorithm>
#include <iterator>


namespace boost_no_std_distance{

int test()
{
   const char* begin = 0;
   const char* end = 0;
   if(std::distance(begin, end)) return -1;
   return 0;
}

}





