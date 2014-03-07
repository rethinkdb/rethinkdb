//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_MS_INT64_NUMERIC_LIMITS
//  TITLE:         std::numeric_limits<__int64>
//  DESCRIPTION:   The C++ implementation does not provide the a specialisation
//                 for std::numeric_limits<__int64>.

#include <limits>

namespace boost_no_ms_int64_numeric_limits{

int test()
{
   if(0 == std::numeric_limits<__int64>::is_specialized) return -1;
   if(0 == std::numeric_limits<unsigned __int64>::is_specialized) return -1;
   return 0;
}

}





