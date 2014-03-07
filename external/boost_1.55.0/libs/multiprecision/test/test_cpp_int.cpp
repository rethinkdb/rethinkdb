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
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/timer.hpp>
#include "test.hpp"

template <class T>
T generate_random(unsigned bits_wanted)
{
   static boost::random::mt19937 gen;
   typedef boost::random::mt19937::result_type random_type;

   T max_val;
   unsigned digits;
   if(std::numeric_limits<T>::is_bounded && (bits_wanted == (unsigned)std::numeric_limits<T>::digits))
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

template <class T>
struct is_checked_cpp_int : public boost::mpl::false_ {};
template <unsigned MinBits, unsigned MaxBits, boost::multiprecision::cpp_integer_type SignType, class Allocator, boost::multiprecision::expression_template_option ET>
struct is_checked_cpp_int<boost::multiprecision::number<boost::multiprecision::cpp_int_backend<MinBits, MaxBits, SignType, boost::multiprecision::checked, Allocator>, ET> > : public boost::mpl::true_ {};

template <class Number>
struct tester
{
   typedef Number test_type;
   typedef typename test_type::backend_type::checked_type checked;

   unsigned last_error_count;
   boost::timer tim;

   boost::multiprecision::mpz_int a, b, c, d;
   int si;
   unsigned ui;
   test_type a1, b1, c1, d1;


   void t1()
   {
      using namespace boost::multiprecision;
      BOOST_CHECK_EQUAL(a.str(), a1.str());
      BOOST_CHECK_EQUAL(b.str(), b1.str());
      BOOST_CHECK_EQUAL(c.str(), c1.str());
      BOOST_CHECK_EQUAL(d.str(), d1.str());
      BOOST_CHECK_EQUAL(mpz_int(a+b).str(), test_type(a1 + b1).str());
      BOOST_CHECK_EQUAL((mpz_int(a)+=b).str(), (test_type(a1) += b1).str());
      BOOST_CHECK_EQUAL((mpz_int(b)+=a).str(), (test_type(b1) += a1).str());
      BOOST_CHECK_EQUAL(mpz_int(a-b).str(), test_type(a1 - b1).str());
      BOOST_CHECK_EQUAL((mpz_int(a)-=b).str(), (test_type(a1) -= b1).str());
      BOOST_CHECK_EQUAL(mpz_int(mpz_int(-a)+b).str(), test_type(test_type(-a1) + b1).str());
      BOOST_CHECK_EQUAL(mpz_int(mpz_int(-a)-b).str(), test_type(test_type(-a1) - b1).str());
      BOOST_CHECK_EQUAL(mpz_int(c * d).str(), test_type(c1 * d1).str());
      BOOST_CHECK_EQUAL((mpz_int(c)*=d).str(), (test_type(c1) *= d1).str());
      BOOST_CHECK_EQUAL((mpz_int(d)*=c).str(), (test_type(d1) *= c1).str());
      BOOST_CHECK_EQUAL(mpz_int(c * -d).str(), test_type(c1 * -d1).str());
      BOOST_CHECK_EQUAL(mpz_int(-c * d).str(), test_type(-c1 * d1).str());
      BOOST_CHECK_EQUAL((mpz_int(c)*=-d).str(), (test_type(c1) *= -d1).str());
      BOOST_CHECK_EQUAL((mpz_int(-d)*=c).str(), (test_type(-d1) *= c1).str());
      BOOST_CHECK_EQUAL(mpz_int(b * c).str(), test_type(b1 * c1).str());
      BOOST_CHECK_EQUAL(mpz_int(a / b).str(), test_type(a1 / b1).str());
      BOOST_CHECK_EQUAL((mpz_int(a)/=b).str(), (test_type(a1) /= b1).str());
      BOOST_CHECK_EQUAL(mpz_int(a / -b).str(), test_type(a1 / -b1).str());
      BOOST_CHECK_EQUAL(mpz_int(-a / b).str(), test_type(-a1 / b1).str());
      BOOST_CHECK_EQUAL((mpz_int(a)/=-b).str(), (test_type(a1) /= -b1).str());
      BOOST_CHECK_EQUAL((mpz_int(-a)/=b).str(), (test_type(-a1) /= b1).str());
      BOOST_CHECK_EQUAL(mpz_int(a / d).str(), test_type(a1 / d1).str());
      BOOST_CHECK_EQUAL(mpz_int(a % b).str(), test_type(a1 % b1).str());
      BOOST_CHECK_EQUAL((mpz_int(a)%=b).str(), (test_type(a1) %= b1).str());
      BOOST_CHECK_EQUAL(mpz_int(a % -b).str(), test_type(a1 % -b1).str());
      BOOST_CHECK_EQUAL((mpz_int(a)%=-b).str(), (test_type(a1) %= -b1).str());
      BOOST_CHECK_EQUAL(mpz_int(-a % b).str(), test_type(-a1 % b1).str());
      BOOST_CHECK_EQUAL((mpz_int(-a)%=b).str(), (test_type(-a1) %= b1).str());
      BOOST_CHECK_EQUAL(mpz_int(a % d).str(), test_type(a1 % d1).str());
      BOOST_CHECK_EQUAL((mpz_int(a)%=d).str(), (test_type(a1) %= d1).str());

      if(!std::numeric_limits<test_type>::is_bounded)
      {
         test_type p = a1 * b1;
         test_type r;
         divide_qr(p, b1, p, r);
         BOOST_CHECK_EQUAL(p, a1);
         BOOST_CHECK_EQUAL(r, test_type(0));

         p = a1 * d1;
         divide_qr(p, d1, p, r);
         BOOST_CHECK_EQUAL(p, a1);
         BOOST_CHECK_EQUAL(r, test_type(0));

         divide_qr(p, test_type(1), p, r);
         BOOST_CHECK_EQUAL(p, a1);
         BOOST_CHECK_EQUAL(r, test_type(0));
      }
   }

   void t2()
   {
      using namespace boost::multiprecision;
      // bitwise ops:
      BOOST_CHECK_EQUAL(mpz_int(a|b).str(), test_type(a1 | b1).str());
      BOOST_CHECK_EQUAL((mpz_int(a)|=b).str(), (test_type(a1) |= b1).str());
      if(!is_checked_cpp_int<test_type>::value)
      {
         BOOST_CHECK_EQUAL(mpz_int(-a|b).str(), test_type(-a1 | b1).str());
         BOOST_CHECK_EQUAL((mpz_int(-a)|=b).str(), (test_type(-a1) |= b1).str());
         BOOST_CHECK_EQUAL(mpz_int(a|-b).str(), test_type(a1 | -b1).str());
         BOOST_CHECK_EQUAL((mpz_int(a)|=-b).str(), (test_type(a1) |= -b1).str());
         BOOST_CHECK_EQUAL(mpz_int(-a|-b).str(), test_type(-a1 | -b1).str());
         BOOST_CHECK_EQUAL((mpz_int(-a)|=-b).str(), (test_type(-a1) |= -b1).str());
      }
      BOOST_CHECK_EQUAL(mpz_int(a&b).str(), test_type(a1 & b1).str());
      BOOST_CHECK_EQUAL((mpz_int(a)&=b).str(), (test_type(a1) &= b1).str());
      if(!is_checked_cpp_int<test_type>::value)
      {
         BOOST_CHECK_EQUAL(mpz_int(-a&b).str(), test_type(-a1 & b1).str());
         BOOST_CHECK_EQUAL((mpz_int(-a)&=b).str(), (test_type(-a1) &= b1).str());
         BOOST_CHECK_EQUAL(mpz_int(a&-b).str(), test_type(a1 & -b1).str());
         BOOST_CHECK_EQUAL((mpz_int(a)&=-b).str(), (test_type(a1) &= -b1).str());
         BOOST_CHECK_EQUAL(mpz_int(-a&-b).str(), test_type(-a1 & -b1).str());
         BOOST_CHECK_EQUAL((mpz_int(-a)&=-b).str(), (test_type(-a1) &= -b1).str());
      }
      BOOST_CHECK_EQUAL(mpz_int(a^b).str(), test_type(a1 ^ b1).str());
      BOOST_CHECK_EQUAL((mpz_int(a)^=b).str(), (test_type(a1) ^= b1).str());
      if(!is_checked_cpp_int<test_type>::value)
      {
         BOOST_CHECK_EQUAL(mpz_int(-a^b).str(), test_type(-a1 ^ b1).str());
         BOOST_CHECK_EQUAL((mpz_int(-a)^=b).str(), (test_type(-a1) ^= b1).str());
         BOOST_CHECK_EQUAL(mpz_int(a^-b).str(), test_type(a1 ^ -b1).str());
         BOOST_CHECK_EQUAL((mpz_int(a)^=-b).str(), (test_type(a1) ^= -b1).str());
         BOOST_CHECK_EQUAL(mpz_int(-a^-b).str(), test_type(-a1 ^ -b1).str());
         BOOST_CHECK_EQUAL((mpz_int(-a)^=-b).str(), (test_type(-a1) ^= -b1).str());
      }
      // Shift ops:
      for(unsigned i = 0; i < 128; ++i)
      {
         if(!std::numeric_limits<test_type>::is_bounded)
         {
            BOOST_CHECK_EQUAL(mpz_int(a << i).str(), test_type(a1 << i).str());
         }
         else if(!is_checked_cpp_int<test_type>::value)
         {
            test_type t1(mpz_int(a << i).str());
            test_type t2 = a1 << i;
            BOOST_CHECK_EQUAL(t1, t2);
         }
         BOOST_CHECK_EQUAL(mpz_int(a >> i).str(), test_type(a1 >> i).str());
      }
      // gcd/lcm
      BOOST_CHECK_EQUAL(mpz_int(gcd(a, b)).str(), test_type(gcd(a1, b1)).str());
      BOOST_CHECK_EQUAL(mpz_int(lcm(c, d)).str(), test_type(lcm(c1, d1)).str());
      BOOST_CHECK_EQUAL(mpz_int(gcd(-a, b)).str(), test_type(gcd(-a1, b1)).str());
      BOOST_CHECK_EQUAL(mpz_int(lcm(-c, d)).str(), test_type(lcm(-c1, d1)).str());
      BOOST_CHECK_EQUAL(mpz_int(gcd(-a, -b)).str(), test_type(gcd(-a1, -b1)).str());
      BOOST_CHECK_EQUAL(mpz_int(lcm(-c, -d)).str(), test_type(lcm(-c1, -d1)).str());
      BOOST_CHECK_EQUAL(mpz_int(gcd(a, -b)).str(), test_type(gcd(a1, -b1)).str());
      BOOST_CHECK_EQUAL(mpz_int(lcm(c, -d)).str(), test_type(lcm(c1, -d1)).str());
      // Integer sqrt:
      mpz_int r;
      test_type r1;
      BOOST_CHECK_EQUAL(sqrt(a, r).str(), sqrt(a1, r1).str());
      BOOST_CHECK_EQUAL(r.str(), r1.str());
   }

   void t3()
   {
      using namespace boost::multiprecision;
      // Now check operations involving signed integers:
      BOOST_CHECK_EQUAL(mpz_int(a + si).str(), test_type(a1 + si).str());
      BOOST_CHECK_EQUAL(mpz_int(a + -si).str(), test_type(a1 + -si).str());
      BOOST_CHECK_EQUAL(mpz_int(-a + si).str(), test_type(-a1 + si).str());
      BOOST_CHECK_EQUAL(mpz_int(si + a).str(), test_type(si + a1).str());
      BOOST_CHECK_EQUAL((mpz_int(a)+=si).str(), (test_type(a1) += si).str());
      BOOST_CHECK_EQUAL((mpz_int(a)+=-si).str(), (test_type(a1) += -si).str());
      BOOST_CHECK_EQUAL((mpz_int(-a)+=si).str(), (test_type(-a1) += si).str());
      BOOST_CHECK_EQUAL((mpz_int(-a)+=-si).str(), (test_type(-a1) += -si).str());
      BOOST_CHECK_EQUAL(mpz_int(a - si).str(), test_type(a1 - si).str());
      BOOST_CHECK_EQUAL(mpz_int(a - -si).str(), test_type(a1 - -si).str());
      BOOST_CHECK_EQUAL(mpz_int(-a - si).str(), test_type(-a1 - si).str());
      BOOST_CHECK_EQUAL(mpz_int(si - a).str(), test_type(si - a1).str());
      BOOST_CHECK_EQUAL((mpz_int(a)-=si).str(), (test_type(a1) -= si).str());
      BOOST_CHECK_EQUAL((mpz_int(a)-=-si).str(), (test_type(a1) -= -si).str());
      BOOST_CHECK_EQUAL((mpz_int(-a)-=si).str(), (test_type(-a1) -= si).str());
      BOOST_CHECK_EQUAL((mpz_int(-a)-=-si).str(), (test_type(-a1) -= -si).str());
      BOOST_CHECK_EQUAL(mpz_int(b * si).str(), test_type(b1 * si).str());
      BOOST_CHECK_EQUAL(mpz_int(b * -si).str(), test_type(b1 * -si).str());
      BOOST_CHECK_EQUAL(mpz_int(-b * si).str(), test_type(-b1 * si).str());
      BOOST_CHECK_EQUAL(mpz_int(si * b).str(), test_type(si * b1).str());
      BOOST_CHECK_EQUAL((mpz_int(a)*=si).str(), (test_type(a1) *= si).str());
      BOOST_CHECK_EQUAL((mpz_int(a)*=-si).str(), (test_type(a1) *= -si).str());
      BOOST_CHECK_EQUAL((mpz_int(-a)*=si).str(), (test_type(-a1) *= si).str());
      BOOST_CHECK_EQUAL((mpz_int(-a)*=-si).str(), (test_type(-a1) *= -si).str());
      BOOST_CHECK_EQUAL(mpz_int(a / si).str(), test_type(a1 / si).str());
      BOOST_CHECK_EQUAL(mpz_int(a / -si).str(), test_type(a1 / -si).str());
      BOOST_CHECK_EQUAL(mpz_int(-a / si).str(), test_type(-a1 / si).str());
      BOOST_CHECK_EQUAL((mpz_int(a)/=si).str(), (test_type(a1) /= si).str());
      BOOST_CHECK_EQUAL((mpz_int(a)/=-si).str(), (test_type(a1) /= -si).str());
      BOOST_CHECK_EQUAL((mpz_int(-a)/=si).str(), (test_type(-a1) /= si).str());
      BOOST_CHECK_EQUAL((mpz_int(-a)/=-si).str(), (test_type(-a1) /= -si).str());
      BOOST_CHECK_EQUAL(mpz_int(a % si).str(), test_type(a1 % si).str());
      BOOST_CHECK_EQUAL(mpz_int(a % -si).str(), test_type(a1 % -si).str());
      BOOST_CHECK_EQUAL(mpz_int(-a % si).str(), test_type(-a1 % si).str());
      BOOST_CHECK_EQUAL((mpz_int(a)%=si).str(), (test_type(a1) %= si).str());
      BOOST_CHECK_EQUAL((mpz_int(a)%=-si).str(), (test_type(a1) %= -si).str());
      BOOST_CHECK_EQUAL((mpz_int(-a)%=si).str(), (test_type(-a1) %= si).str());
      BOOST_CHECK_EQUAL((mpz_int(-a)%=-si).str(), (test_type(-a1) %= -si).str());
      if((si > 0) || !is_checked_cpp_int<test_type>::value)
      {
         BOOST_CHECK_EQUAL(mpz_int(a|si).str(), test_type(a1 | si).str());
         BOOST_CHECK_EQUAL((mpz_int(a)|=si).str(), (test_type(a1) |= si).str());
         BOOST_CHECK_EQUAL(mpz_int(a&si).str(), test_type(a1 & si).str());
         BOOST_CHECK_EQUAL((mpz_int(a)&=si).str(), (test_type(a1) &= si).str());
         BOOST_CHECK_EQUAL(mpz_int(a^si).str(), test_type(a1 ^ si).str());
         BOOST_CHECK_EQUAL((mpz_int(a)^=si).str(), (test_type(a1) ^= si).str());
         BOOST_CHECK_EQUAL(mpz_int(si|a).str(), test_type(si|a1).str());
         BOOST_CHECK_EQUAL(mpz_int(si&a).str(), test_type(si&a1).str());
         BOOST_CHECK_EQUAL(mpz_int(si^a).str(), test_type(si^a1).str());
      }
      BOOST_CHECK_EQUAL(mpz_int(gcd(a, si)).str(), test_type(gcd(a1, si)).str());
      BOOST_CHECK_EQUAL(mpz_int(gcd(si, b)).str(), test_type(gcd(si, b1)).str());
      BOOST_CHECK_EQUAL(mpz_int(lcm(c, si)).str(), test_type(lcm(c1, si)).str());
      BOOST_CHECK_EQUAL(mpz_int(lcm(si, d)).str(), test_type(lcm(si, d1)).str());
      BOOST_CHECK_EQUAL(mpz_int(gcd(-a, si)).str(), test_type(gcd(-a1, si)).str());
      BOOST_CHECK_EQUAL(mpz_int(gcd(-si, b)).str(), test_type(gcd(-si, b1)).str());
      BOOST_CHECK_EQUAL(mpz_int(lcm(-c, si)).str(), test_type(lcm(-c1, si)).str());
      BOOST_CHECK_EQUAL(mpz_int(lcm(-si, d)).str(), test_type(lcm(-si, d1)).str());
      BOOST_CHECK_EQUAL(mpz_int(gcd(-a, -si)).str(), test_type(gcd(-a1, -si)).str());
      BOOST_CHECK_EQUAL(mpz_int(gcd(-si, -b)).str(), test_type(gcd(-si, -b1)).str());
      BOOST_CHECK_EQUAL(mpz_int(lcm(-c, -si)).str(), test_type(lcm(-c1, -si)).str());
      BOOST_CHECK_EQUAL(mpz_int(lcm(-si, -d)).str(), test_type(lcm(-si, -d1)).str());
      BOOST_CHECK_EQUAL(mpz_int(gcd(a, -si)).str(), test_type(gcd(a1, -si)).str());
      BOOST_CHECK_EQUAL(mpz_int(gcd(si, -b)).str(), test_type(gcd(si, -b1)).str());
      BOOST_CHECK_EQUAL(mpz_int(lcm(c, -si)).str(), test_type(lcm(c1, -si)).str());
      BOOST_CHECK_EQUAL(mpz_int(lcm(si, -d)).str(), test_type(lcm(si, -d1)).str());
   }

   void t4()
   {
      using namespace boost::multiprecision;
      // Now check operations involving unsigned integers:
      BOOST_CHECK_EQUAL(mpz_int(a + ui).str(), test_type(a1 + ui).str());
      BOOST_CHECK_EQUAL(mpz_int(-a + ui).str(), test_type(-a1 + ui).str());
      BOOST_CHECK_EQUAL(mpz_int(ui + a).str(), test_type(ui + a1).str());
      BOOST_CHECK_EQUAL((mpz_int(a)+=ui).str(), (test_type(a1) += ui).str());
      BOOST_CHECK_EQUAL((mpz_int(-a)+=ui).str(), (test_type(-a1) += ui).str());
      BOOST_CHECK_EQUAL(mpz_int(a - ui).str(), test_type(a1 - ui).str());
      BOOST_CHECK_EQUAL(mpz_int(-a - ui).str(), test_type(-a1 - ui).str());
      BOOST_CHECK_EQUAL(mpz_int(ui - a).str(), test_type(ui - a1).str());
      BOOST_CHECK_EQUAL((mpz_int(a)-=ui).str(), (test_type(a1) -= ui).str());
      BOOST_CHECK_EQUAL((mpz_int(-a)-=ui).str(), (test_type(-a1) -= ui).str());
      BOOST_CHECK_EQUAL(mpz_int(b * ui).str(), test_type(b1 * ui).str());
      BOOST_CHECK_EQUAL(mpz_int(-b * ui).str(), test_type(-b1 * ui).str());
      BOOST_CHECK_EQUAL(mpz_int(ui * b).str(), test_type(ui * b1).str());
      BOOST_CHECK_EQUAL((mpz_int(a)*=ui).str(), (test_type(a1) *= ui).str());
      BOOST_CHECK_EQUAL((mpz_int(-a)*=ui).str(), (test_type(-a1) *= ui).str());
      BOOST_CHECK_EQUAL(mpz_int(a / ui).str(), test_type(a1 / ui).str());
      BOOST_CHECK_EQUAL(mpz_int(-a / ui).str(), test_type(-a1 / ui).str());
      BOOST_CHECK_EQUAL((mpz_int(a)/=ui).str(), (test_type(a1) /= ui).str());
      BOOST_CHECK_EQUAL((mpz_int(-a)/=ui).str(), (test_type(-a1) /= ui).str());
      BOOST_CHECK_EQUAL(mpz_int(a % ui).str(), test_type(a1 % ui).str());
      BOOST_CHECK_EQUAL(mpz_int(-a % ui).str(), test_type(-a1 % ui).str());
      BOOST_CHECK_EQUAL((mpz_int(a)%=ui).str(), (test_type(a1) %= ui).str());
      BOOST_CHECK_EQUAL((mpz_int(-a)%=ui).str(), (test_type(-a1) %= ui).str());
      BOOST_CHECK_EQUAL(mpz_int(a|ui).str(), test_type(a1 | ui).str());
      BOOST_CHECK_EQUAL((mpz_int(a)|=ui).str(), (test_type(a1) |= ui).str());
      BOOST_CHECK_EQUAL(mpz_int(a&ui).str(), test_type(a1 & ui).str());
      BOOST_CHECK_EQUAL((mpz_int(a)&=ui).str(), (test_type(a1) &= ui).str());
      BOOST_CHECK_EQUAL(mpz_int(a^ui).str(), test_type(a1 ^ ui).str());
      BOOST_CHECK_EQUAL((mpz_int(a)^=ui).str(), (test_type(a1) ^= ui).str());
      BOOST_CHECK_EQUAL(mpz_int(ui|a).str(), test_type(ui|a1).str());
      BOOST_CHECK_EQUAL(mpz_int(ui&a).str(), test_type(ui&a1).str());
      BOOST_CHECK_EQUAL(mpz_int(ui^a).str(), test_type(ui^a1).str());
      BOOST_CHECK_EQUAL(mpz_int(gcd(a, ui)).str(), test_type(gcd(a1, ui)).str());
      BOOST_CHECK_EQUAL(mpz_int(gcd(ui, b)).str(), test_type(gcd(ui, b1)).str());
      BOOST_CHECK_EQUAL(mpz_int(lcm(c, ui)).str(), test_type(lcm(c1, ui)).str());
      BOOST_CHECK_EQUAL(mpz_int(lcm(ui, d)).str(), test_type(lcm(ui, d1)).str());
      BOOST_CHECK_EQUAL(mpz_int(gcd(-a, ui)).str(), test_type(gcd(-a1, ui)).str());
      BOOST_CHECK_EQUAL(mpz_int(lcm(-c, ui)).str(), test_type(lcm(-c1, ui)).str());
      BOOST_CHECK_EQUAL(mpz_int(gcd(ui, -b)).str(), test_type(gcd(ui, -b1)).str());
      BOOST_CHECK_EQUAL(mpz_int(lcm(ui, -d)).str(), test_type(lcm(ui, -d1)).str());

      if(std::numeric_limits<test_type>::is_modulo && checked::value)
      {
         static mpz_int m = mpz_int(1) << std::numeric_limits<test_type>::digits;
         mpz_int t(a);
         test_type t1(a1);
         for(unsigned i = 0; i < 10; ++i)
         {
            t *= a;
            t %= m;
            t += a;
            t %= m;
            t1 *= a1;
            t1 += a1;
         }
         BOOST_CHECK_EQUAL(t.str(), t1.str());
      }
   }

   void t5()
   {
      using namespace boost::multiprecision;
      //
      // Now integer functions:
      //
      mpz_int z1, z2;
      test_type t1, t2;
      divide_qr(a, b, z1, z2);
      divide_qr(a1, b1, t1, t2);
      BOOST_CHECK_EQUAL(z1.str(), t1.str());
      BOOST_CHECK_EQUAL(z2.str(), t2.str());
      BOOST_CHECK_EQUAL(integer_modulus(a, si), integer_modulus(a1, si));
      BOOST_CHECK_EQUAL(lsb(a), lsb(a1));
      BOOST_CHECK_EQUAL(msb(a), msb(a1));

      for(unsigned i = 0; i < 1000; i += 13)
      {
         BOOST_CHECK_EQUAL(bit_test(a, i), bit_test(a1, i));
      }
      if(!std::numeric_limits<test_type>::is_modulo)
      {
         // We have to take care that our powers don't grow too large, otherwise this takes "forever",
         // also don't test for modulo types, as these may give a different result from arbitrary
         // precision types:
         BOOST_CHECK_EQUAL(mpz_int(pow(d, ui % 19)).str(), test_type(pow(d1, ui % 19)).str());
         BOOST_CHECK_EQUAL(mpz_int(powm(a, b, c)).str(), test_type(powm(a1, b1, c1)).str());
         BOOST_CHECK_EQUAL(mpz_int(powm(a, b, ui)).str(), test_type(powm(a1, b1, ui)).str());
         BOOST_CHECK_EQUAL(mpz_int(powm(a, ui, c)).str(), test_type(powm(a1, ui, c1)).str());
      }
      BOOST_CHECK_EQUAL(lsb(a), lsb(a1));
      BOOST_CHECK_EQUAL(msb(a), msb(a1));
   }

   void test_bug_cases()
   {
      if(!std::numeric_limits<test_type>::is_bounded)
      {
         // https://svn.boost.org/trac/boost/ticket/7878
         test_type a("0x1000000000000000000000000000000000000000000000000000000000000000");
         test_type b = 0xFFFFFFFF;
         test_type c = a * b + b;  // quotient has 1 in the final place
         test_type q, r;
         divide_qr(c, b, q, r);
         BOOST_CHECK_EQUAL(a + 1, q);
         BOOST_CHECK_EQUAL(r, 0);

         b = static_cast<test_type>("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
         c = a * b + b;  // quotient has 1 in the final place
         divide_qr(c, b, q, r);
         BOOST_CHECK_EQUAL(a + 1, q);
         BOOST_CHECK_EQUAL(r, 0);
         //
         // Not a bug, but test some other special cases that don't otherwise occur through
         // random testing:
         //
         c = a * b; // quotient has zero in the final place
         divide_qr(c, b, q, r);
         BOOST_CHECK_EQUAL(q, a);
         BOOST_CHECK_EQUAL(r, 0);
         divide_qr(c, a, q, r);
         BOOST_CHECK_EQUAL(q, b);
         BOOST_CHECK_EQUAL(r, 0);
         ++c;
         divide_qr(c, b, q, r);
         BOOST_CHECK_EQUAL(q, a);
         BOOST_CHECK_EQUAL(r, 1);
      }
      // Bug https://svn.boost.org/trac/boost/ticket/8126:
      test_type a("-4294967296");
      test_type b("4294967296");
      test_type c("-1");
      a = (a / b);
      BOOST_CHECK_EQUAL(a, -1);
      a = -4294967296;
      a = (a / b) * c;
      BOOST_CHECK_EQUAL(a, 1);
      a = -23;
      b = 23;
      a = (a / b) * c;
      BOOST_CHECK_EQUAL(a, 1);
      a = -23;
      a = (a / b) / c;
      BOOST_CHECK_EQUAL(a, 1);
      a = test_type("-26607734784073568386365259775");
      b = test_type("8589934592");
      a = a / b;
      BOOST_CHECK_EQUAL(a, test_type("-3097548007973652377"));
      // Bug https://svn.boost.org/trac/boost/ticket/8133:
      a = test_type("0x12345600012434ffffffffffffffffffffffff");
      unsigned ui = 0xffffffff;
      a = a - ui;
      BOOST_CHECK_EQUAL(a, test_type("0x12345600012434ffffffffffffffff00000000"));
      a = test_type("0x12345600012434ffffffffffffffffffffffff");
#ifndef BOOST_NO_LONG_LONG
      unsigned long long ull = 0xffffffffffffffffuLL;
      a = a - ull;
      BOOST_CHECK_EQUAL(a, test_type("0x12345600012434ffffffff0000000000000000"));
#endif
      //
      // Now check that things which should be zero really are
      // https://svn.boost.org/trac/boost/ticket/8145:
      //
      a = -1;
      a += 1;
      BOOST_CHECK_EQUAL(a, 0);
      a = 1;
      a += -1;
      BOOST_CHECK_EQUAL(a, 0);
      a = -1;
      a += test_type(1);
      BOOST_CHECK_EQUAL(a, 0);
      a = 1;
      a += test_type(-1);
      BOOST_CHECK_EQUAL(a, 0);
      a = test_type("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
      a -= test_type("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
      BOOST_CHECK_EQUAL(a, 0);
      a = -test_type("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
      a += test_type("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
      BOOST_CHECK_EQUAL(a, 0);
      a = 2;
      a *= 0;
      BOOST_CHECK_EQUAL(a, 0);
      a = -2;
      a *= 0;
      BOOST_CHECK_EQUAL(a, 0);
      a = 2;
      a *= test_type(0);
      BOOST_CHECK_EQUAL(a, 0);
      a = -2;
      a *= test_type(0);
      BOOST_CHECK_EQUAL(a, 0);
      a = -2;
      a /= 50;
      BOOST_CHECK_EQUAL(a, 0);
      a = -test_type("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
      a /= (1 + test_type("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"));
      BOOST_CHECK_EQUAL(a, 0);
      // https://svn.boost.org/trac/boost/ticket/8160
      a = 1;
      a = 0 / test_type(1);
      BOOST_CHECK_EQUAL(a, 0);
      a = 1;
      a = 0 % test_type(25);
      BOOST_CHECK_EQUAL(a, 0);
   }

   void test()
   {
      using namespace boost::multiprecision;

      test_bug_cases();

      last_error_count = 0;

      BOOST_CHECK_EQUAL(Number(), 0);

      for(int i = 0; i < 10000; ++i)
      {
         a = generate_random<mpz_int>(1000);
         b = generate_random<mpz_int>(512);
         c = generate_random<mpz_int>(256);
         d = generate_random<mpz_int>(32);

         si = d.convert_to<int>();
         ui = si;

         a1 = static_cast<test_type>(a.str());
         b1 = static_cast<test_type>(b.str());
         c1 = static_cast<test_type>(c.str());
         d1 = static_cast<test_type>(d.str());

         t1();
         t2();
#ifndef SLOW_COMPILER
         t3();
         t4();
         t5();
#endif

         if(last_error_count != (unsigned)boost::detail::test_errors())
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
            std::cout << "-a1 + b1 = " << test_type(-a1)+b1 << std::endl;
            std::cout << "-a - b   = " << mpz_int(-a)-b << std::endl;
            std::cout << "-a1 - b1 = " << test_type(-a1)-b1 << std::endl;
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

         //
         // Check to see if test is taking too long.
         // Tests run on the compiler farm time out after 300 seconds,
         // so don't get too close to that:
         //
         if(tim.elapsed() > 200)
         {
            std::cout << "Timeout reached, aborting tests now....\n";
            break;
         }

      }
   }
};

#if !defined(TEST1) && !defined(TEST2) && !defined(TEST3)
#define TEST1
#define TEST2
#define TEST3
#endif

int main()
{
   using namespace boost::multiprecision;
#ifdef TEST1
   tester<cpp_int> t1;
   t1.test();
#endif
#ifdef TEST2
   tester<number<cpp_int_backend<2048, 2048, signed_magnitude, checked, void> > > t2;
   t2.test();
#endif
#ifdef TEST3
   // Unchecked test verifies modulo arithmetic:
   tester<number<cpp_int_backend<2048, 2048, signed_magnitude, unchecked, void> > > t3;
   t3.test();
#endif
   return boost::report_errors();
}



