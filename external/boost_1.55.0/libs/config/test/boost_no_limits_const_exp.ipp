//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
//  TITLE:         compile time constants in <limits>
//  DESCRIPTION:   constants such as numeric_limits<T>::is_signed
//                 are not available for use at compile-time.

#include <limits>

namespace boost_no_limits_compile_time_constants{

struct UDT{};

template <int i>
struct assert_ice
{
   enum { value = i };
};

int test()
{
   assert_ice< ::std::numeric_limits<int>::is_signed > one;
   assert_ice< ::std::numeric_limits<double>::is_specialized > two;
   assert_ice< ::std::numeric_limits<UDT>::is_specialized > three;
   assert_ice< ::std::numeric_limits<UDT>::is_signed > four;
   (void)one;
   (void)two;
   (void)three;
   (void)four;
   return 0;
}

}




