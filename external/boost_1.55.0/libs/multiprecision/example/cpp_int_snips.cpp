///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include <boost/multiprecision/cpp_int.hpp>
#include <iostream>

void t1()
{
   //[cpp_int_eg
   //=#include <boost/multiprecision/cpp_int.hpp>

   using namespace boost::multiprecision;

   int128_t v = 1;

   // Do some fixed precision arithmetic:
   for(unsigned i = 1; i <= 20; ++i)
      v *= i;

   std::cout << v << std::endl; // prints 20!

   // Repeat at arbitrary precision:
   cpp_int u = 1;
   for(unsigned i = 1; i <= 100; ++i)
      u *= i;

   std::cout << u << std::endl; // prints 100!

   //]
}

void t3()
{
   //[cpp_rational_eg
   //=#include <boost/multiprecision/cpp_int.hpp>

   using namespace boost::multiprecision;

   cpp_rational v = 1;

   // Do some arithmetic:
   for(unsigned i = 1; i <= 1000; ++i)
      v *= i;
   v /= 10;

   std::cout << v << std::endl; // prints 1000! / 10
   std::cout << numerator(v) << std::endl;
   std::cout << denominator(v) << std::endl;

   cpp_rational w(2, 3);  // component wise constructor
   std::cout << w << std::endl; // prints 2/3
   //]
}


int main()
{
   t1();
   t3();
   return 0;
}

