//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_LONG_LONG_NUMERIC_LIMITS
//  TITLE:         std::numeric_limits<long long>
//  DESCRIPTION:   The C++ implementation does not provide the a specialisation
//                 for std::numeric_limits<long long>.

#include <limits>

namespace boost_no_long_long_numeric_limits{

int test()
{
#ifdef __GNUC__
__extension__
#endif
   typedef long long llt;
#ifdef __GNUC__
__extension__
#endif
   typedef unsigned long long ullt;
   if(0 == std::numeric_limits<llt>::is_specialized) return -1;
   if(0 == std::numeric_limits<ullt>::is_specialized) return -1;
   return 0;
}

}





