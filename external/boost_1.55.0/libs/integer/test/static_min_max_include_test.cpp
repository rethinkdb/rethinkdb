//  Copyright John Maddock 2009.  
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/integer/static_min_max.hpp> // must be the only #include!

int main()
{
   boost::static_min_max_signed_type m = boost::static_signed_min<2, 3>::value + boost::static_signed_max<2, 3>::value;
   (void)m;
   boost::static_min_max_unsigned_type u = boost::static_unsigned_min<2, 3>::value + boost::static_unsigned_max<2, 3>::value;
   (void)u;
}
