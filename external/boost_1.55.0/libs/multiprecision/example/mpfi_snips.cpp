///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include <boost/multiprecision/mpfi.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <iostream>

void t1()
{
   //[mpfi_eg
   //=#include <boost/multiprecision/mpfi.hpp>

   using namespace boost::multiprecision;

   // Operations at variable precision and no numeric_limits support:
   mpfi_float a = 2;
   mpfi_float::default_precision(1000);
   std::cout << mpfi_float::default_precision() << std::endl;
   std::cout << sqrt(a) << std::endl; // print root-2

   // Operations at fixed precision and full numeric_limits support:
   mpfi_float_100 b = 2;
   std::cout << std::numeric_limits<mpfi_float_100>::digits << std::endl;
   // We can use any C++ std lib function:
   std::cout << log(b) << std::endl; // print log(2)

   // Access the underlying data:
   mpfi_t r;
   mpfi_init(r);
   mpfi_set(r, b.backend().data());

   // Construct some explicit intervals and perform set operations:
   mpfi_float_50 i1(1, 2), i2(1.5, 2.5);
   std::cout << intersect(i1, i2) << std::endl;
   std::cout << hull(i1, i2) << std::endl;
   std::cout << overlap(i1, i2) << std::endl;
   std::cout << subset(i1, i2) << std::endl;
   //]
   mpfi_clear(r);
}

int main()
{
   t1();
   return 0;
}



