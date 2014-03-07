//  Copyright John Maddock 2009.  
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/integer.hpp> // must be the only #include!

int main()
{
   boost::int_fast_t<char>::fast f = 0;
   (void)f;
   boost::int_t<16>::fast f2 = 0;
   (void)f2;
   boost::int_t<32>::exact e = 0;
   (void)e;
   boost::int_t<12>::least l = 0;
   (void)l;
   boost::uint_t<16>::fast uf2 = 0;
   (void)uf2;
   boost::uint_t<32>::exact ue = 0;
   (void)ue;
   boost::uint_t<12>::least ul = 0;
   (void)ul;
   boost::int_max_value_t<200>::fast v1 = 0;
   (void)v1;
   boost::int_max_value_t<2000>::least v2 = 0;
   (void)v2;
   boost::int_min_value_t<-200>::fast v3 = 0;
   (void)v3;
   boost::int_min_value_t<-2000>::least v4 = 0;
   (void)v4;
   boost::uint_value_t<200>::fast v5 = 0;
   (void)v5;
   boost::uint_value_t<2000>::least v6 = 0;
   (void)v6;
}
