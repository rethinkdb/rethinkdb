///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include <boost/multiprecision/mpfr.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <iostream>

void t1()
{
   //[mpfr_eg
   //=#include <boost/multiprecision/mpfr.hpp>

   using namespace boost::multiprecision;

   // Operations at variable precision and no numeric_limits support:
   mpfr_float a = 2;
   mpfr_float::default_precision(1000);
   std::cout << mpfr_float::default_precision() << std::endl;
   std::cout << sqrt(a) << std::endl; // print root-2

   // Operations at fixed precision and full numeric_limits support:
   mpfr_float_100 b = 2;
   std::cout << std::numeric_limits<mpfr_float_100>::digits << std::endl;
   // We can use any C++ std lib function:
   std::cout << log(b) << std::endl; // print log(2)
   // We can also use any function from Boost.Math:
   std::cout << boost::math::tgamma(b) << std::endl;
   // These even work when the argument is an expression template:
   std::cout << boost::math::tgamma(b * b) << std::endl;

   // Access the underlying data:
   mpfr_t r;
   mpfr_init(r);
   mpfr_set(r, b.backend().data(), GMP_RNDN);
   //]
   mpfr_clear(r);
}

int main()
{
   t1();
   return 0;
}



