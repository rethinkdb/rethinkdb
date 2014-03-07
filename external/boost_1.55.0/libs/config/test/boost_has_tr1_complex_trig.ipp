//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_TR1_COMPLEX_INVERSE_TRIG
//  TITLE:         std::complex inverse trig functions
//  DESCRIPTION:   The std lib has a tr1-conforming set of std::complex inverse trig functions.

#include <complex>

namespace boost_has_tr1_complex_inverse_trig{


int test()
{
   std::complex<double> cd;
   std::asin(cd);
   std::acos(cd);
   std::atan(cd);   
   std::asinh(cd);
   std::acosh(cd);
   std::atanh(cd); 
   std::fabs(cd);  
   return 0;
}

}
