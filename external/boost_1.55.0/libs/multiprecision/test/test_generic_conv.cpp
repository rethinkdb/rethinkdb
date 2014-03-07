///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_
//

#ifdef _MSC_VER
#  define _SCL_SECURE_NO_WARNINGS
#endif

#include <boost/detail/lightweight_test.hpp>
#include <boost/array.hpp>
#include "test.hpp"

#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/random.hpp>

#ifdef TEST_GMP
#include <boost/multiprecision/gmp.hpp>
#endif
#ifdef TEST_TOMMATH
#include <boost/multiprecision/tommath.hpp>
#endif
#ifdef TEST_MPFR
#include <boost/multiprecision/mpfr.hpp>
#endif

int main()
{
   using namespace boost::multiprecision;
   using namespace boost::random;

   independent_bits_engine<mt11213b, 1024, cpp_int> gen;

   for(unsigned i = 0; i < 100; ++i)
   {
      cpp_int c = gen();
      //
      // Integer to integer conversions first:
      //
#ifdef TEST_GMP
      mpz_int z(c);
      cpp_int t(z);
      BOOST_CHECK_EQUAL(t, c);
      z.assign(-c);
      t.assign(-z);
      BOOST_CHECK_EQUAL(t, c);
#endif
#ifdef TEST_TOMMATH
      tom_int tom(c);
      cpp_int t2(tom);
      BOOST_CHECK_EQUAL(t2, c);
      tom.assign(-c);
      t2.assign(-tom);
      BOOST_CHECK_EQUAL(t2, c);
#endif
      //
      // Now integer to float:
      //
      typedef number<cpp_dec_float<500> > dec_float_500;
      dec_float_500 df(c);
      dec_float_500 df2(c.str());
      BOOST_CHECK_EQUAL(df, df2);
      df.assign(-c);
      df2 = -df2;
      BOOST_CHECK_EQUAL(df, df2);
#ifdef TEST_GMP
      typedef number<gmp_float<500> > mpf_type;
      mpf_type mpf(c);
      mpf_type mpf2(c.str());
      BOOST_CHECK_EQUAL(mpf, mpf2);
      mpf.assign(-c);
      mpf2 = -mpf2;
      BOOST_CHECK_EQUAL(mpf, mpf2);
#endif
#ifdef TEST_MPFR
      typedef number<mpfr_float_backend<500> > mpfr_type;
      mpfr_type mpfr(c);
      mpfr_type mpfr2(c.str());
      BOOST_CHECK_EQUAL(mpfr, mpfr2);
      mpfr.assign(-c);
      mpfr2 = -mpfr2;
      BOOST_CHECK_EQUAL(mpfr, mpfr2);
#endif
      //
      // Now float to float:
      //
      df.assign(c);
      df /= dec_float_500(gen());
      dec_float_500 tol("1e-500");
#ifdef TEST_GMP
      mpf.assign(df);
      mpf2 = static_cast<mpf_type>(df.str());
      BOOST_CHECK_EQUAL(mpf, mpf2);
      df.assign(mpf);
      df2 = static_cast<dec_float_500>(mpf.str());
      BOOST_CHECK(fabs((df - df2) / df) < tol);
#endif
#ifdef TEST_MPFR
      mpfr.assign(df);
      mpfr2 = static_cast<mpfr_type>(df.str());
      BOOST_CHECK_EQUAL(mpfr, mpfr2);
      df.assign(mpfr);
      df2 = static_cast<dec_float_500>(mpfr.str());
      BOOST_CHECK(fabs((df - df2) / df) < tol);
#endif
      //
      // Rational to rational conversions:
      //
      cpp_rational cppr(c, gen());
#ifdef TEST_GMP
      mpq_rational mpq(cppr);
      cpp_rational cppr2(mpq);
      BOOST_CHECK_EQUAL(cppr, cppr2);
#endif
#ifdef TEST_TOMMATH
      tom_rational tr(cppr);
      cpp_rational cppr3(tr);
      BOOST_CHECK_EQUAL(cppr, cppr3);
#endif
      //
      // Integer to rational conversions:
      //
#ifdef TEST_GMP
      mpq.assign(c);
      mpq_rational mpq2 = static_cast<mpq_rational>(c.str());
      BOOST_CHECK_EQUAL(mpq, mpq2);
#endif
#ifdef TEST_TOMMATH
      tr.assign(c);
      tom_rational tr2 = static_cast<tom_rational>(c.str());
      BOOST_CHECK_EQUAL(tr, tr2);
#endif
      //
      // Rational to float:
      //
      df.assign(cppr);
      df2.assign(numerator(cppr));
      df2 /= dec_float_500(denominator(cppr));
      BOOST_CHECK(fabs(df - df2) / df2 < tol);
   }

   return boost::report_errors();
}



