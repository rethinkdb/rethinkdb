///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include <boost/multiprecision/gmp.hpp>
#include <boost/multiprecision/random.hpp>
#include <iostream>
#include <iomanip>

void t1()
{
   //[random_eg1
   //=#include <boost/multiprecision/gmp.hpp>
   //=#include <boost/multiprecision/random.hpp>

   using namespace boost::multiprecision;
   using namespace boost::random;

   //
   // Declare our random number generator type, the underlying generator
   // is the Mersenne twister mt19937 engine, and 256 bits are generated:
   //
   typedef independent_bits_engine<mt19937, 256, mpz_int> generator_type;
   generator_type gen;
   //
   // Generate some values:
   //
   std::cout << std::hex << std::showbase;
   for(unsigned i = 0; i < 10; ++i)
      std::cout << gen() << std::endl;
   //]
}

void t2()
{
   //[random_eg2
   //=#include <boost/multiprecision/gmp.hpp>
   //=#include <boost/multiprecision/random.hpp>

   using namespace boost::multiprecision;
   using namespace boost::random;

   //
   // Generate integers in a given range using uniform_int,
   // the underlying generator is invoked multiple times
   // to generate enough bits:
   //
   mt19937 mt;
   uniform_int_distribution<mpz_int> ui(0, mpz_int(1) << 256);
   //
   // Generate the numbers:
   //
   std::cout << std::hex << std::showbase;
   for(unsigned i = 0; i < 10; ++i)
      std::cout << ui(mt) << std::endl;

   //]
}

void t3()
{
   //[random_eg3
   //=#include <boost/multiprecision/gmp.hpp>
   //=#include <boost/multiprecision/random.hpp>

   using namespace boost::multiprecision;
   using namespace boost::random;
   //
   // We need an underlying generator with at least as many bits as the
   // floating point type to generate numbers in [0, 1) with all the bits
   // in the floating point type randomly filled:
   //
   uniform_01<mpf_float_50> uf;
   independent_bits_engine<mt19937, 50L*1000L/301L, mpz_int> gen;
   //
   // Generate the values:
   //
   std::cout << std::setprecision(50);
   for(unsigned i = 0; i < 20; ++i)
      std::cout << uf(gen) << std::endl;
   //]
}

void t4()
{
   //[random_eg4
   //=#include <boost/multiprecision/gmp.hpp>
   //=#include <boost/multiprecision/random.hpp>

   using namespace boost::multiprecision;
   using namespace boost::random;
   //
   // We can repeat the above example, with other distributions:
   //
   uniform_real_distribution<mpf_float_50> ur(-20, 20);
   gamma_distribution<mpf_float_50> gd(20);
   independent_bits_engine<mt19937, 50L*1000L/301L, mpz_int> gen;
   //
   // Generate some values:
   //
   std::cout << std::setprecision(50);
   for(unsigned i = 0; i < 20; ++i)
      std::cout << ur(gen) << std::endl;
   for(unsigned i = 0; i < 20; ++i)
      std::cout << gd(gen) << std::endl;
   //]
}

int main()
{
   t1();
   t2();
   t3();
   t4();
   return 0;
}

