//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_TR1_RANDOM
//  TITLE:         std::tr1::random
//  DESCRIPTION:   The std lib has a tr1-conforming random numer library.

#include <random>

namespace boost_has_tr1_random{

using std::tr1::variate_generator;

int test()
{
   return 0;
}

}
