//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_TR1_COMPLEX_OVERLOADS
//  TITLE:         std::complex overloads
//  DESCRIPTION:   The std lib has a tr1-conforming set of std::complex overloads.

#include <complex>

namespace boost_has_tr1_complex_overloads{


int test()
{
   std::arg(0);
   std::conj(0.0);   
   return 0;
}

}
