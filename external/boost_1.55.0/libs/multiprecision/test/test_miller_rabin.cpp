///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#ifdef _MSC_VER
#  define _SCL_SECURE_NO_WARNINGS
#endif

#include <boost/multiprecision/gmp.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/miller_rabin.hpp>
#include <boost/math/special_functions/prime.hpp>
#include <iostream>
#include <iomanip>
#include "test.hpp"


template <class I>
void test()
{
   //
   // Very simple test program to verify that the GMP's Miller-Rabin
   // implementation and this one agree on whether some random numbers
   // are prime or not.  Of course these are probabilistic tests so there's
   // no reason why they should actually agree - except the probability of
   // disagreement for 25 trials is almost infinitely small.
   //
   using namespace boost::random;
   using namespace boost::multiprecision;

   typedef I test_type;

   static const unsigned test_bits = 
      std::numeric_limits<test_type>::digits && (std::numeric_limits<test_type>::digits <= 256)
      ? std::numeric_limits<test_type>::digits
      : 128;

   independent_bits_engine<mt11213b, test_bits, test_type> gen;
   //
   // We must use a different generator for the tests and number generation, otherwise
   // we get false positives.  Further we use the same random number engine for the
   // Miller Rabin test as GMP uses internally:
   //
   mt19937 gen2;

   //
   // Begin by testing the primes in our table as all these should return true:
   //
   for(unsigned i = 1; i < boost::math::max_prime; ++i)
   {
      BOOST_TEST(miller_rabin_test(test_type(boost::math::prime(i)), 25, gen));
      BOOST_TEST(mpz_probab_prime_p(mpz_int(boost::math::prime(i)).backend().data(), 25));
   }
   //
   // Now test some random values and compare GMP's native routine with ours.
   //
   for(unsigned i = 0; i < 10000; ++i)
   {
      test_type n = gen();
      bool is_prime_boost = miller_rabin_test(n, 25, gen2);
      bool is_gmp_prime = mpz_probab_prime_p(mpz_int(n).backend().data(), 25) ? true : false;
      if(is_prime_boost && is_gmp_prime)
      {
         std::cout << "We have a prime: " << std::hex << std::showbase << n << std::endl;
      }
      if(is_prime_boost != is_gmp_prime)
         std::cout << std::hex << std::showbase << "n = " << n << std::endl;
      BOOST_CHECK_EQUAL(is_prime_boost, is_gmp_prime);
   }
}

int main()
{
   using namespace boost::multiprecision;

   test<mpz_int>();
   test<number<gmp_int, et_off> >();
   test<boost::uint64_t>();
   test<boost::uint32_t>();

   test<cpp_int>();
   test<number<cpp_int_backend<64, 64, unsigned_magnitude, checked, void>, et_off> >();
   test<checked_uint128_t>();
   test<checked_uint1024_t>();

   return boost::report_errors();
}


