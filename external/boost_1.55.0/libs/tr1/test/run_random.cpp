//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef TEST_STD_HEADERS
#include <random>
#else
#include <boost/tr1/random.hpp>
#endif

#include <boost/test/test_tools.hpp>
#include <boost/test/included/test_exec_monitor.hpp>

#include <iostream>
#include <iomanip>

template<class PRNG, class R>
void validate(const std::string & name, PRNG &rng, R expected)
{
   typedef typename PRNG::result_type result_type;
   std::cout << "validating " << name << ": ";
   for(int i = 0; i < 9999; i++)
   {
      result_type r = rng();
      BOOST_CHECK(r >= (rng.min)());
      BOOST_CHECK(r <= (rng.max)());
   }
   result_type val = rng();
   BOOST_CHECK(val >= (rng.min)());
   BOOST_CHECK(val <= (rng.max)());
   // Get the result:
   bool result = val == expected;

   // allow for a simple eyeball check for MSVC instantiation brokenness
   // (if the numbers for all generators are the same, it's obviously broken)
   if(std::numeric_limits<R>::is_integer == 0)
      std::cout << std::setprecision(std::numeric_limits<R>::digits10 + 1);
   std::cout << val;
   if(result == 0)
   {
      std::cout << "   (Expected: ";
      if(std::numeric_limits<R>::is_integer == 0)
         std::cout << std::setprecision(std::numeric_limits<R>::digits10 + 1);
      std::cout << expected << ")";
   }
   std::cout << std::endl;
   BOOST_CHECK(result);
}

struct counting_functor
{
   typedef unsigned result_type;
   unsigned operator()()
   {
      return i++;
   }
   counting_functor(int j)
      : i(j){}
private:
   int i;
};

int test_main(int, char*[])
{
   do{
      typedef std::tr1::linear_congruential< ::boost::int32_t, 16807, 0, 2147483647> minstd_rand0;
      minstd_rand0 a1;
      minstd_rand0 a2(a1);
      validate("minstd_rand0", a1, 1043618065u);
      a1 = a2;
      validate("minstd_rand0", a2, 1043618065u);
      validate("minstd_rand0", a1, 1043618065u);
      a1.seed(1);
      counting_functor f1(1);
      a2.seed(f1);
      validate("minstd_rand0", a2, 1043618065u);
      validate("minstd_rand0", a1, 1043618065u);
      a1 = minstd_rand0(1);
      counting_functor f2(1);
      a2 = minstd_rand0(f2);
      validate("minstd_rand0", a2, 1043618065u);
      validate("minstd_rand0", a1, 1043618065u);
   }while(0);

   do{
      typedef std::tr1::linear_congruential< ::boost::uint32_t, 69069, 0, 0> mt_seed;
      typedef std::tr1::mersenne_twister< ::boost::uint32_t,32,351,175,19,0xccab8ee7,11,7,0x31b6ab00,15,0xffe50000,17> mt11213b;
      mt11213b a1;
      mt11213b a2(a1);
      validate("mt11213b", a1, 3809585648u);
      a1 = a2;
      validate("mt11213b", a2, 3809585648u);
      validate("mt11213b", a1, 3809585648u);
      a1.seed(5489UL);
      validate("mt11213b", a1, 3809585648u);
      a1 = mt11213b(5489u);
      validate("mt11213b", a1, 3809585648u);
      mt_seed s1(4357UL);
      a1.seed(s1);
      validate("mt11213b", a1, 2742893714u);
      mt_seed s2(4357UL);
      mt11213b a3(s2);
      validate("mt11213b", a3, 2742893714u);
   }while(0);

   do{
      typedef std::tr1::linear_congruential< :: boost::int32_t, 40014, 0, 2147483563> seed_t;
      typedef std::tr1::subtract_with_carry< ::boost::int32_t, (1<<24), 10, 24> sub_t;
      sub_t a1;
      sub_t a2(a1);
      validate("subtract_with_carry", a1, 7937952u);
      a1 = a2;
      validate("subtract_with_carry", a2, 7937952u);
      validate("subtract_with_carry", a1, 7937952u);
      a1.seed(0);
      seed_t s1(19780503UL);
      a2.seed(s1);
      validate("subtract_with_carry", a2, 7937952u);
      validate("subtract_with_carry", a1, 7937952u);
      a1 = sub_t(0);
      seed_t s2(19780503UL);
      a2 = sub_t(s2);
      validate("subtract_with_carry", a2, 7937952u);
      validate("subtract_with_carry", a1, 7937952u);
   }while(0);

   do{
      typedef std::tr1::linear_congruential< :: boost::int32_t, 40014, 0, 2147483563> seed_t;
      typedef std::tr1::subtract_with_carry_01< float, 24, 10, 24> ranlux_base_01;
      ranlux_base_01 a1;
      ranlux_base_01 a2(a1);
      validate("ranlux_base_01", a1, 0.4731388F);
      a1 = a2;
      validate("ranlux_base_01", a2, 0.4731388F);
      validate("ranlux_base_01", a1, 0.4731388F);
      a1.seed(0);
      seed_t s1(19780503UL);
      a2.seed(s1);
      validate("ranlux_base_01", a2, 0.4731388F);
      validate("ranlux_base_01", a1, 0.4731388F);
      a1 = ranlux_base_01(0);
      seed_t s2(19780503UL);
      a2 = ranlux_base_01(s2);
      validate("ranlux_base_01", a2, 0.4731388F);
      validate("ranlux_base_01", a1, 0.4731388F);
   }while(0);

   do{
      typedef std::tr1::linear_congruential< :: boost::int32_t, 40014, 0, 2147483563> seed_t;
      typedef std::tr1::subtract_with_carry_01< double, 48, 10, 24> ranlux64_base_01;
      ranlux64_base_01 a1;
      ranlux64_base_01 a2(a1);
      validate("ranlux64_base_01", a1,  0.1332451100961265);
      a1 = a2;
      validate("ranlux64_base_01", a2,  0.1332451100961265);
      validate("ranlux64_base_01", a1,  0.1332451100961265);
      a1.seed(0);
      seed_t s1(19780503UL);
      a2.seed(s1);
      validate("ranlux64_base_01", a2,  0.1332451100961265);
      validate("ranlux64_base_01", a1,  0.1332451100961265);
      a1 = ranlux64_base_01(0);
      seed_t s2(19780503UL);
      a2 = ranlux64_base_01(s2);
      validate("ranlux64_base_01", a2,  0.1332451100961265);
      validate("ranlux64_base_01", a1,  0.1332451100961265);
   }while(0);

   do{
      typedef std::tr1::linear_congruential< :: boost::int32_t, 40014, 0, 2147483563> seed_t;
      typedef std::tr1::subtract_with_carry< ::boost::int32_t, (1<<24), 10, 24> sub_t;
      typedef std::tr1::xor_combine<sub_t, 0, sub_t, 3> xor_t;
      xor_t a1;
      validate("xor_combine", a1,  61989536u);
      a1 = xor_t();
      xor_t a2(a1);
      validate("xor_combine", a1,  61989536u);
      a1 = a2;
      validate("xor_combine", a2,  61989536u);
      validate("xor_combine", a1,  61989536u);
      a1.seed(0);
      seed_t s1(19780503UL);
      a2.seed(s1);
      validate("xor_combine", a2,  90842400u);
      validate("xor_combine", a1,  114607192u);
      a1 = xor_t(0);
      seed_t s2(19780503UL);
      a2 = xor_t(s2);
      validate("xor_combine", a2,  90842400u);
      validate("xor_combine", a1,  114607192u);
   }while(0);

   do{
      std::tr1::minstd_rand0 r1;
      validate("std::tr1::minstd_rand0", r1, 1043618065u);
      std::tr1::minstd_rand r2;
      validate("std::tr1::minstd_rand", r2, 399268537u);
      std::tr1::mt19937 r3;
      validate("std::tr1::mt19937", r3, 4123659995u);
      std::tr1::ranlux3 r4;
      validate("std::tr1::ranlux3", r4, 5957620u);
      std::tr1::ranlux4 r5;
      validate("std::tr1::ranlux4", r5, 8587295u);
      std::tr1::ranlux3_01 r6;
      validate("std::tr1::ranlux3_01", r6,  5957620.0F/std::pow(2.0f,24));
      std::tr1::ranlux4_01 r7;
      validate("std::tr1::ranlux4_01", r7, 8587295.0F/std::pow(2.0f,24));
   }while(0);

   return 0;
}

