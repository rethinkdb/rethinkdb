//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_INTEGRAL_INT64_T
//  TITLE:         long long and integral constant expressions
//  DESCRIPTION:   The platform supports long long in integral constant expressions.

#include <cstdlib>


namespace boost_no_integral_int64_t{

#ifdef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
enum{ mask = 1uLL << 50 };

template <unsigned long long m>
struct llt
{
   enum{ value = m };
};
#else
#ifdef __GNUC__
__extension__
#endif
static const unsigned long long mask = 1uLL << 50;

#ifdef __GNUC__
__extension__
#endif
template <unsigned long long m>
struct llt
{
#ifdef __GNUC__
__extension__
#endif
   static const unsigned long long value = m;
};
#endif

int test()
{
   return llt<mask>::value != (1uLL << 50);
}

}





