//  (C) Copyright John Maddock and Dave Abrahams 2002. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_DEDUCED_TYPENAME
//  TITLE:         deduced typenames
//  DESCRIPTION:   Some compilers can't use the typename keyword in deduced contexts.

#ifndef BOOST_DEDUCED_TYPENAME
#define BOOST_DEDUCED_TYPENAME typename
#endif


namespace boost_deduced_typename{

template <class T>
int f(T const volatile*, BOOST_DEDUCED_TYPENAME T::type* = 0)
{
        return 0;
}

struct X { typedef int type; };

int test()
{
   return f((X*)0);
}

}








