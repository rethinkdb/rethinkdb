//  (C) Copyright Peter Dimov 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_TYPEID
//  TITLE:         typeid unavailable
//  DESCRIPTION:   The compiler does not support typeid in this mode

#include <typeinfo>

namespace boost_no_typeid
{

int test()
{
   (void)typeid(int);
   return 0;
}

}

