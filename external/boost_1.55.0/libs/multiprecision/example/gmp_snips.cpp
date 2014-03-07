///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include <boost/multiprecision/gmp.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <iostream>

void t1()
{
   //[mpz_eg
   //=#include <boost/multiprecision/gmp.hpp>

   using namespace boost::multiprecision;

   mpz_int v = 1;

   // Do some arithmetic:
   for(unsigned i = 1; i <= 1000; ++i)
      v *= i;

   std::cout << v << std::endl; // prints 1000!

   // Access the underlying representation:
   mpz_t z;
   mpz_init(z);
   mpz_set(z, v.backend().data());
   //]
   mpz_clear(z);
}

void t2()
{
   //[mpf_eg
   //=#include <boost/multiprecision/gmp.hpp>

   using namespace boost::multiprecision;

   // Operations at variable precision and limited standard library support:
   mpf_float a = 2;
   mpf_float::default_precision(1000);
   std::cout << mpf_float::default_precision() << std::endl;
   std::cout << sqrt(a) << std::endl; // print root-2

   // Operations at fixed precision and full standard library support:
   mpf_float_100 b = 2;
   std::cout << std::numeric_limits<mpf_float_100>::digits << std::endl;
   // We can use any C++ std lib function:
   std::cout << log(b) << std::endl; // print log(2)
   // We can also use any function from Boost.Math:
   std::cout << boost::math::tgamma(b) << std::endl;
   // These even work when the argument is an expression template:
   std::cout << boost::math::tgamma(b * b) << std::endl;

   // Access the underlying representation:
   mpf_t f;
   mpf_init(f);
   mpf_set(f, a.backend().data());
   //]
   mpf_clear(f);
}

void t3()
{
   //[mpq_eg
   //=#include <boost/multiprecision/gmp.hpp>

   using namespace boost::multiprecision;

   mpq_rational v = 1;

   // Do some arithmetic:
   for(unsigned i = 1; i <= 1000; ++i)
      v *= i;
   v /= 10;

   std::cout << v << std::endl; // prints 1000! / 10
   std::cout << numerator(v) << std::endl;
   std::cout << denominator(v) << std::endl;

   mpq_rational w(2, 3);  // component wise constructor
   std::cout << w << std::endl; // prints 2/3

   // Access the underlying data:
   mpq_t q;
   mpq_init(q);
   mpq_set(q, v.backend().data());
   //]
   mpq_clear(q);
}

int main()
{
   t1();
   t2();
   t3();
   return 0;
}

