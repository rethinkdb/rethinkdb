//  Copyright John Maddock 2009.  
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/integer/static_log2.hpp> // must be the only #include!

int main()
{
   boost::static_log2_argument_type arg = 0;
   (void)arg;
   boost::static_log2_result_type result = boost::static_log2<30>::value;
   (void)result;
}
