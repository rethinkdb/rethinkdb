///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

//
// Compare arithmetic results using fixed_int to GMP results.
//

#ifdef _MSC_VER
#  define _SCL_SECURE_NO_WARNINGS
#endif

#include <boost/multiprecision/gmp.hpp>
#include <boost/multiprecision/fixed_int.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include "test.hpp"

template <class T>
T generate_random(unsigned bits_wanted)
{
   static boost::random::mt19937 gen;
   typedef boost::random::mt19937::result_type random_type;

   T max_val;
   unsigned digits;
   if(std::numeric_limits<T>::is_bounded && (bits_wanted == std::numeric_limits<T>::digits))
   {
      max_val = (std::numeric_limits<T>::max)();
      digits = std::numeric_limits<T>::digits;
   }
   else
   {
      max_val = T(1) << bits_wanted;
      digits = bits_wanted;
   }

   unsigned bits_per_r_val = std::numeric_limits<random_type>::digits - 1;
   while((random_type(1) << bits_per_r_val) > (gen.max)()) --bits_per_r_val;

   unsigned terms_needed = digits / bits_per_r_val + 1;

   T val = 0;
   for(unsigned i = 0; i < terms_needed; ++i)
   {
      val *= (gen.max)();
      val += gen();
   }
   val %= max_val;
   return val;
}

int main()
{
   using namespace boost::multiprecision;
   typedef number<fixed_int<1024, true> > packed_type;
   unsigned last_error_count = 0;
   for(int i = 0; i < 1000; ++i)
   {
      mpz_int a = generate_random<mpz_int>(1000);
      mpz_int b = generate_random<mpz_int>(512);
      mpz_int c = generate_random<mpz_int>(256);
      mpz_int d = generate_random<mpz_int>(32);

      int si = d.convert_to<int>();

      packed_type a1 = a.str();
      packed_type b1 = b.str();
      packed_type c1 = c.str();
      packed_type d1 = d.str();

      BOOST_CHECK_EQUAL(a.str(), a1.str());
      BOOST_CHECK_EQUAL(b.str(), b1.str());
      BOOST_CHECK_EQUAL(c.str(), c1.str());
      BOOST_CHECK_EQUAL(d.str(), d1.str());
      BOOST_CHECK_EQUAL(mpz_int(a+b).str(), packed_type(a1 + b1).str());
      BOOST_CHECK_EQUAL(mpz_int(a-b).str(), packed_type(a1 - b1).str());
      BOOST_CHECK_EQUAL(mpz_int(mpz_int(-a)+b).str(), packed_type(packed_type(-a1) + b1).str());
      BOOST_CHECK_EQUAL(mpz_int(mpz_int(-a)-b).str(), packed_type(packed_type(-a1) - b1).str());
      BOOST_CHECK_EQUAL(mpz_int(c * d).str(), packed_type(c1 * d1).str());
      BOOST_CHECK_EQUAL(mpz_int(c * -d).str(), packed_type(c1 * -d1).str());
      BOOST_CHECK_EQUAL(mpz_int(-c * d).str(), packed_type(-c1 * d1).str());
      BOOST_CHECK_EQUAL(mpz_int(b * c).str(), packed_type(b1 * c1).str());
      BOOST_CHECK_EQUAL(mpz_int(a / b).str(), packed_type(a1 / b1).str());
      BOOST_CHECK_EQUAL(mpz_int(a / -b).str(), packed_type(a1 / -b1).str());
      BOOST_CHECK_EQUAL(mpz_int(-a / b).str(), packed_type(-a1 / b1).str());
      BOOST_CHECK_EQUAL(mpz_int(a / d).str(), packed_type(a1 / d1).str());
      BOOST_CHECK_EQUAL(mpz_int(a % b).str(), packed_type(a1 % b1).str());
      BOOST_CHECK_EQUAL(mpz_int(a % -b).str(), packed_type(a1 % -b1).str());
      BOOST_CHECK_EQUAL(mpz_int(-a % b).str(), packed_type(-a1 % b1).str());
      BOOST_CHECK_EQUAL(mpz_int(a % d).str(), packed_type(a1 % d1).str());
      // bitwise ops:
      BOOST_CHECK_EQUAL(mpz_int(a|b).str(), packed_type(a1 | b1).str());
      BOOST_CHECK_EQUAL(mpz_int(a&b).str(), packed_type(a1 & b1).str());
      BOOST_CHECK_EQUAL(mpz_int(a^b).str(), packed_type(a1 ^ b1).str());
      // Now check operations involving integers:
      BOOST_CHECK_EQUAL(mpz_int(a + si).str(), packed_type(a1 + si).str());
      BOOST_CHECK_EQUAL(mpz_int(a + -si).str(), packed_type(a1 + -si).str());
      BOOST_CHECK_EQUAL(mpz_int(-a + si).str(), packed_type(-a1 + si).str());
      BOOST_CHECK_EQUAL(mpz_int(si + a).str(), packed_type(si + a1).str());
      BOOST_CHECK_EQUAL(mpz_int(a - si).str(), packed_type(a1 - si).str());
      BOOST_CHECK_EQUAL(mpz_int(a - -si).str(), packed_type(a1 - -si).str());
      BOOST_CHECK_EQUAL(mpz_int(-a - si).str(), packed_type(-a1 - si).str());
      BOOST_CHECK_EQUAL(mpz_int(si - a).str(), packed_type(si - a1).str());
      BOOST_CHECK_EQUAL(mpz_int(b * si).str(), packed_type(b1 * si).str());
      BOOST_CHECK_EQUAL(mpz_int(b * -si).str(), packed_type(b1 * -si).str());
      BOOST_CHECK_EQUAL(mpz_int(-b * si).str(), packed_type(-b1 * si).str());
      BOOST_CHECK_EQUAL(mpz_int(si * b).str(), packed_type(si * b1).str());
      BOOST_CHECK_EQUAL(mpz_int(a / si).str(), packed_type(a1 / si).str());
      BOOST_CHECK_EQUAL(mpz_int(a / -si).str(), packed_type(a1 / -si).str());
      BOOST_CHECK_EQUAL(mpz_int(-a / si).str(), packed_type(-a1 / si).str());
      BOOST_CHECK_EQUAL(mpz_int(a % si).str(), packed_type(a1 % si).str());
      BOOST_CHECK_EQUAL(mpz_int(a % -si).str(), packed_type(a1 % -si).str());
      BOOST_CHECK_EQUAL(mpz_int(-a % si).str(), packed_type(-a1 % si).str());
      BOOST_CHECK_EQUAL(mpz_int(a|si).str(), packed_type(a1 | si).str());
      BOOST_CHECK_EQUAL(mpz_int(a&si).str(), packed_type(a1 & si).str());
      BOOST_CHECK_EQUAL(mpz_int(a^si).str(), packed_type(a1 ^ si).str());
      BOOST_CHECK_EQUAL(mpz_int(si|a).str(), packed_type(si|a1).str());
      BOOST_CHECK_EQUAL(mpz_int(si&a).str(), packed_type(si&a1).str());
      BOOST_CHECK_EQUAL(mpz_int(si^a).str(), packed_type(si^a1).str());
      BOOST_CHECK_EQUAL(mpz_int(gcd(a, b)).str(), packed_type(gcd(a1, b1)).str());
      BOOST_CHECK_EQUAL(mpz_int(lcm(c, d)).str(), packed_type(lcm(c1, d1)).str());

      if(last_error_count != boost::detail::test_errors())
      {
         last_error_count = boost::detail::test_errors();
         std::cout << std::hex << std::showbase;

         std::cout << "a    = " << a << std::endl;
         std::cout << "a1   = " << a1 << std::endl;
         std::cout << "b    = " << b << std::endl;
         std::cout << "b1   = " << b1 << std::endl;
         std::cout << "c    = " << c << std::endl;
         std::cout << "c1   = " << c1 << std::endl;
         std::cout << "d    = " << d << std::endl;
         std::cout << "d1   = " << d1 << std::endl;
         std::cout << "a + b   = " << a+b << std::endl;
         std::cout << "a1 + b1 = " << a1+b1 << std::endl;
         std::cout << std::dec;
         std::cout << "a - b   = " << a-b << std::endl;
         std::cout << "a1 - b1 = " << a1-b1 << std::endl;
         std::cout << "-a + b   = " << mpz_int(-a)+b << std::endl;
         std::cout << "-a1 + b1 = " << packed_type(-a1)+b1 << std::endl;
         std::cout << "-a - b   = " << mpz_int(-a)-b << std::endl;
         std::cout << "-a1 - b1 = " << packed_type(-a1)-b1 << std::endl;
         std::cout << "c*d    = " << c*d << std::endl;
         std::cout << "c1*d1  = " << c1*d1 << std::endl;
         std::cout << "b*c    = " << b*c << std::endl;
         std::cout << "b1*c1  = " << b1*c1 << std::endl;
         std::cout << "a/b    = " << a/b << std::endl;
         std::cout << "a1/b1  = " << a1/b1 << std::endl;
         std::cout << "a/d    = " << a/d << std::endl;
         std::cout << "a1/d1  = " << a1/d1 << std::endl;
         std::cout << "a%b    = " << a%b << std::endl;
         std::cout << "a1%b1  = " << a1%b1 << std::endl;
         std::cout << "a%d    = " << a%d << std::endl;
         std::cout << "a1%d1  = " << a1%d1 << std::endl;
      }
   }
   return boost::report_errors();
}



