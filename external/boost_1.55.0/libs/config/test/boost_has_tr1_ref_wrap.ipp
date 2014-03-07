//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_TR1_REFERENCE_WRAPPER
//  TITLE:         std::tr1::reference_wrapper
//  DESCRIPTION:   The std lib has a tr1-conforming reference_wrapper.

#include <functional>

namespace boost_has_tr1_reference_wrapper{

int test()
{
   int i;
   std::tr1::reference_wrapper<int> r = std::tr1::ref(i);
   (void)r;
   return 0;
}

}
