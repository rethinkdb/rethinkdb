//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_LIMITS
//  TITLE:         <limits>
//  DESCRIPTION:   The C++ implementation does not provide the <limits> header.

#include <limits>

namespace boost_no_limits{

int test()
{
   if(0 == std::numeric_limits<int>::is_specialized) return -1;
   if(0 == std::numeric_limits<long>::is_specialized) return -1;
   if(0 == std::numeric_limits<char>::is_specialized) return -1;
   if(0 == std::numeric_limits<unsigned char>::is_specialized) return -1;
   if(0 == std::numeric_limits<signed char>::is_specialized) return -1;
   if(0 == std::numeric_limits<float>::is_specialized) return -1;
   if(0 == std::numeric_limits<double>::is_specialized) return -1;
   if(0 == std::numeric_limits<long double>::is_specialized) return -1;
   return 0;
}

}





