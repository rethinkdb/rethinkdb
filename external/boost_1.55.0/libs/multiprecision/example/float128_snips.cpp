///////////////////////////////////////////////////////////////
//  Copyright 2013 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include <boost/multiprecision/float128.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <iostream>

void t1()
{
   //[float128_eg
   //=#include <boost/multiprecision/float128.hpp>

   using namespace boost::multiprecision;

   // Operations at 128-bit precision and full numeric_limits support:
   float128 b = 2;
   // There are 113-bits of precision:
   std::cout << std::numeric_limits<float128>::digits << std::endl;
   // Or 34 decimal places:
   std::cout << std::numeric_limits<float128>::digits10 << std::endl;
   // We can use any C++ std lib function, lets print all the digits as well:
   std::cout << std::setprecision(std::numeric_limits<float128>::max_digits10) 
      << log(b) << std::endl; // print log(2) = 0.693147180559945309417232121458176575
   // We can also use any function from Boost.Math:
   std::cout << boost::math::tgamma(b) << std::endl;
   // And since we have an extended exponent range we can generate some really large 
   // numbers here (4.02387260077093773543702433923004111e+2564):
   std::cout << boost::math::tgamma(float128(1000)) << std::endl;
   //
   // We can declare constants using GCC or Intel's native types, and the Q suffix,
   // these can be declared constexpr if required:
   /*<-*/
#ifndef BOOST_NO_CONSTEXPR
   /*->*/
   constexpr float128 pi = 3.1415926535897932384626433832795028841971693993751058Q;
   /*<-*/
#endif
   /*->*/
   //]
}

int main()
{
   t1();
   return 0;
}

