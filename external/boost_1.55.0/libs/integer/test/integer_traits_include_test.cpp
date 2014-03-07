//  Copyright John Maddock 2009.  
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/integer_traits.hpp> // must be the only #include!

template <class T>
void check_numeric_limits_derived(const std::numeric_limits<T>&){}

template <class T>
void check()
{
   typedef boost::integer_traits<T> traits;
   check_numeric_limits_derived(traits());
   bool b = traits::is_integral;
   (void)b;
   T v = traits::const_min + traits::const_max;
   (void)v;
}

int main()
{
   check<signed char>();
   check<unsigned char>();
   check<char>();
   check<short>();
   check<unsigned short>();
   check<int>();
   check<unsigned int>();
   check<signed long>();
   check<unsigned long>();
#if !defined(BOOST_NO_INTEGRAL_INT64_T) && defined(BOOST_HAS_LONG_LONG)
   check<boost::long_long_type>();
   check<boost::ulong_long_type>();
#endif
}
