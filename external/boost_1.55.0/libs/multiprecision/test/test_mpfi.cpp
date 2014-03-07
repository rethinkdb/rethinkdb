///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#ifdef _MSC_VER
#  define _SCL_SECURE_NO_WARNINGS
#endif

#include "test.hpp"
#include <boost/multiprecision/mpfi.hpp>
#include <boost/multiprecision/random.hpp>

using namespace boost::multiprecision;
using namespace boost::random;

void test_exp()
{
   std::cout << "Testing exp\n";

   mpfr_float_50 val = 1.25;

   for(unsigned i = 0; i < 2000; ++i)
   {
      mpfr_float_100 a(val);
      mpfr_float_100 b = exp(a);
      mpfi_float_50 in = val;
      in = exp(in);
      BOOST_CHECK((boost::math::isfinite)(in));
      BOOST_CHECK(lower(in) <= b);
      BOOST_CHECK(upper(in) >= b);
      b = log(a);
      in = val;
      in = log(in);
      BOOST_CHECK((boost::math::isfinite)(in));
      BOOST_CHECK(lower(in) <= b);
      BOOST_CHECK(upper(in) >= b);
      val *= 1.01;
   }
   val = 1;
   for(unsigned i = 0; i < 2000; ++i)
   {
      mpfr_float_100 a(val);
      mpfr_float_100 b = exp(a);
      mpfi_float_50 in = val;
      in = exp(in);
      BOOST_CHECK((boost::math::isfinite)(in));
      BOOST_CHECK(lower(in) <= b);
      BOOST_CHECK(upper(in) >= b);
      b = log(a);
      in = val;
      in = log(in);
      BOOST_CHECK((boost::math::isfinite)(in));
      BOOST_CHECK(lower(in) <= b);
      BOOST_CHECK(upper(in) >= b);
      val /= 1.01;
   }
}

void test_pow()
{
   std::cout << "Testing pow function\n";

   mt19937 gen;
   uniform_real_distribution<mpfr_float_50> dist1(0, 400);

   for(unsigned i = 0; i < 5000; ++i)
   {
      mpfr_float_50 base, p;
      base = dist1(gen);
      p = dist1(gen);
      mpfr_float_100 a, b, r;
      a = base;
      b = p;
      r = pow(a, b);
      mpfi_float_50 ai, bi, ri;
      ai = base;
      bi = p;
      ri = pow(ai, bi);
      BOOST_CHECK((boost::math::isfinite)(ri));
      BOOST_CHECK(lower(ri) <= r);
      BOOST_CHECK(upper(ri) >= r);
   }
}

void test_trig()
{
   std::cout << "Testing trig functions\n";

   mt19937 gen;
   uniform_real_distribution<mpfr_float_50> dist1(-1.57079632679, 1.57079632679);
   uniform_real_distribution<mpfr_float_50> dist2(-1, 1);

   for(unsigned i = 0; i < 5000; ++i)
   {
      mpfr_float_50 val;
      val = dist1(gen);
      mpfr_float_100 a = val;
      mpfr_float_100 b = sin(a);
      mpfi_float_50 a2 = val;
      mpfi_float_50 b2 = sin(a2);
      BOOST_CHECK((boost::math::isfinite)(b2));
      BOOST_CHECK(lower(b2) <= b);
      BOOST_CHECK(upper(b2) >= b);
      b = cos(a);
      b2 = cos(a2);
      BOOST_CHECK((boost::math::isfinite)(b2));
      BOOST_CHECK(lower(b2) <= b);
      BOOST_CHECK(upper(b2) >= b);
      b = tan(a);
      b2 = tan(a2);
      BOOST_CHECK((boost::math::isfinite)(b2));
      BOOST_CHECK(lower(b2) <= b);
      BOOST_CHECK(upper(b2) >= b);
   }
   for(unsigned i = 0; i < 5000; ++i)
   {
      mpfr_float_50 val;
      val = dist2(gen);
      mpfr_float_100 a = val;
      mpfr_float_100 b = asin(a);
      mpfi_float_50 a2 = val;
      mpfi_float_50 b2 = asin(a2);
      BOOST_CHECK((boost::math::isfinite)(b2));
      BOOST_CHECK(lower(b2) <= b);
      BOOST_CHECK(upper(b2) >= b);
      b = acos(a);
      b2 = acos(a2);
      BOOST_CHECK((boost::math::isfinite)(b2));
      BOOST_CHECK(lower(b2) <= b);
      BOOST_CHECK(upper(b2) >= b);
      b = atan(a);
      b2 = atan(a2);
      BOOST_CHECK((boost::math::isfinite)(b2));
      BOOST_CHECK(lower(b2) <= b);
      BOOST_CHECK(upper(b2) >= b);
   }
}

void test_hyp()
{
   std::cout << "Testing hyperbolic trig functions\n";

   mt19937 gen;
   uniform_real_distribution<mpfr_float_50> dist1(-10, 10);
   uniform_real_distribution<mpfr_float_50> dist2(-1, 1);

   for(unsigned i = 0; i < 5000; ++i)
   {
      mpfr_float_50 val;
      val = dist1(gen);
      mpfr_float_100 a = val;
      mpfr_float_100 b = sinh(a);
      mpfi_float_50 a2 = val;
      mpfi_float_50 b2 = sinh(a2);
      BOOST_CHECK((boost::math::isfinite)(b2));
      BOOST_CHECK(lower(b2) <= b);
      BOOST_CHECK(upper(b2) >= b);
      b = cosh(a);
      b2 = cosh(a2);
      BOOST_CHECK((boost::math::isfinite)(b2));
      BOOST_CHECK(lower(b2) <= b);
      BOOST_CHECK(upper(b2) >= b);
      b = tanh(a);
      b2 = tanh(a2);
      BOOST_CHECK((boost::math::isfinite)(b2));
      BOOST_CHECK(lower(b2) <= b);
      BOOST_CHECK(upper(b2) >= b);
   }
}

void test_intervals()
{
   mpfi_float_50 a(1, 2);
   mpfi_float_50 b(1.5, 2.5);
   BOOST_CHECK_EQUAL(lower(a), 1);
   BOOST_CHECK_EQUAL(upper(a), 2);
   BOOST_CHECK_EQUAL(median(a), 1.5);
   BOOST_CHECK_EQUAL(width(a), 1);
   mpfi_float_50 r = intersect(a, b);
   BOOST_CHECK_EQUAL(lower(r), 1.5);
   BOOST_CHECK_EQUAL(upper(r), 2);
   r = hull(a, b);
   BOOST_CHECK_EQUAL(lower(r), 1);
   BOOST_CHECK_EQUAL(upper(r), 2.5);
   BOOST_CHECK(overlap(a, b));
   BOOST_CHECK(in(mpfr_float_50(1.5), a));
   BOOST_CHECK(in(mpfr_float_50(1), a));
   BOOST_CHECK(in(mpfr_float_50(2), a));
   BOOST_CHECK(!zero_in(a));
   b = mpfi_float_50(1.5, 1.75);
   BOOST_CHECK(subset(b, a));
   BOOST_CHECK(proper_subset(b, a));
   BOOST_CHECK(!empty(a));
   BOOST_CHECK(!singleton(a));
   b = mpfi_float_50(5, 6);
   r = intersect(a, b);
   BOOST_CHECK(empty(r));
}

#ifdef TEST_SPECIAL
#include "math/table_type.hpp"
#include <boost/math/special_functions.hpp>

#define T mpfi_float_50

typedef number<mpfi_float_backend<25> > mpfi_float_25;

void test_log1p_expm1()
{
#  include "../../math/test/log1p_expm1_data.ipp"

   std::cout << std::setprecision(std::numeric_limits<mpfi_float_25>::max_digits10);
   std::cout << "Testing log1p and expm1\n";

   for(unsigned i = 0; i < log1p_expm1_data.size(); ++i)
   {
      mpfi_float_25 in(log1p_expm1_data[i][0]);
      mpfi_float_25 out = boost::math::log1p(in);
      mpfi_float_25 expected(log1p_expm1_data[i][1]);
      if(!subset(expected, out))
      {
         std::cout << in << std::endl;
         std::cout << out << std::endl;
         std::cout << expected << std::endl;
         BOOST_CHECK(lower(out) <= lower(expected));
         BOOST_CHECK(upper(out) >= upper(expected));
      }
      out = boost::math::expm1(in);
      expected = mpfi_float_25(log1p_expm1_data[i][2]);
      if(!subset(expected, out))
      {
         std::cout << in << std::endl;
         std::cout << out << std::endl;
         std::cout << expected << std::endl;
         BOOST_CHECK(lower(out) <= lower(expected));
         BOOST_CHECK(upper(out) >= upper(expected));
      }
   }
}

void test_bessel()
{
#include "../../math/test/bessel_i_int_data.ipp"
#include "../../math/test/bessel_i_data.ipp"

   std::cout << std::setprecision(std::numeric_limits<mpfi_float_25>::max_digits10);
   std::cout << "Testing Bessel Functions\n";

   for(unsigned i = 0; i < bessel_i_int_data.size(); ++i)
   {
      int v = boost::lexical_cast<int>(static_cast<const char*>(bessel_i_int_data[i][0]));
      mpfi_float_25 in(bessel_i_int_data[i][1]);
      mpfi_float_25 out = boost::math::cyl_bessel_i(v, in);
      mpfi_float_25 expected(bessel_i_int_data[i][2]);
      if(!subset(expected, out))
      {
         std::cout << in << std::endl;
         std::cout << out << std::endl;
         std::cout << expected << std::endl;
         BOOST_CHECK(lower(out) <= lower(expected));
         BOOST_CHECK(upper(out) >= upper(expected));
      }
   }
}

#endif

int main()
{
#ifdef TEST_SPECIAL
   test_log1p_expm1();
   test_bessel();
#endif
   test_intervals();
   test_exp();
   test_pow();
   test_trig();
   test_hyp();
   return boost::report_errors();
}



