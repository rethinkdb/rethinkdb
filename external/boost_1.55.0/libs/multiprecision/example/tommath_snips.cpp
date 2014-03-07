///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include <boost/multiprecision/tommath.hpp>
#include <iostream>

void t1()
{
   //[tommath_eg
   //=#include <boost/multiprecision/tommath.hpp>

   boost::multiprecision::tom_int v = 1;

   // Do some arithmetic:
   for(unsigned i = 1; i <= 1000; ++i)
      v *= i;

   std::cout << v << std::endl; // prints 1000!
   std::cout << std::hex << v << std::endl; // prints 1000! in hex format

   try{
      std::cout << std::hex << -v << std::endl; // Ooops! can't print a negative value in hex format!
   }
   catch(const std::runtime_error& e)
   {
      std::cout << e.what() << std::endl;
   }

   try{
      // v is not a 2's complement type, bitwise operations are only supported
      // on positive values:
      v = -v & 2;
   }
   catch(const std::runtime_error& e)
   {
      std::cout << e.what() << std::endl;
   }

   //]
}

void t3()
{
   //[mp_rat_eg
   //=#include <boost/multiprecision/tommath.hpp>

   using namespace boost::multiprecision;

   tom_rational v = 1;

   // Do some arithmetic:
   for(unsigned i = 1; i <= 1000; ++i)
      v *= i;
   v /= 10;

   std::cout << v << std::endl; // prints 1000! / 10
   std::cout << numerator(v) << std::endl;
   std::cout << denominator(v) << std::endl;

   tom_rational w(2, 3); // Component wise constructor
   std::cout << w << std::endl; // prints 2/3

   //]
}

int main()
{
   t1();
   return 0;
}

