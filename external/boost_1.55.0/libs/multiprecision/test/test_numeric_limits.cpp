///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#ifdef _MSC_VER
#  define _SCL_SECURE_NO_WARNINGS
#endif

#include "test.hpp"

#if !defined(TEST_MPF_50) && !defined(TEST_MPF) && !defined(TEST_BACKEND) && !defined(TEST_MPZ) && \
   !defined(TEST_CPP_DEC_FLOAT) && !defined(TEST_MPFR) && !defined(TEST_MPFR_50) && !defined(TEST_MPQ) && \
   !defined(TEST_TOMMATH) && !defined(TEST_CPP_INT) && !defined(TEST_MPFI_50) &&!defined(TEST_FLOAT128)
#  define TEST_MPF_50
#  define TEST_MPF
#  define TEST_BACKEND
#  define TEST_MPZ
#  define TEST_MPFR
#  define TEST_MPFR_50
#  define TEST_CPP_DEC_FLOAT
#  define TEST_MPQ
#  define TEST_TOMMATH
#  define TEST_CPP_INT
#  define TEST_MPFI_50
#  define TEST_FLOAT128

#ifdef _MSC_VER
#pragma message("CAUTION!!: No backend type specified so testing everything.... this will take some time!!")
#endif
#ifdef __GNUC__
#pragma warning "CAUTION!!: No backend type specified so testing everything.... this will take some time!!"
#endif

#endif

#if defined(TEST_MPF_50) || defined(TEST_MPF) || defined(TEST_MPZ) || defined(TEST_MPQ)
#include <boost/multiprecision/gmp.hpp>
#endif
#ifdef TEST_BACKEND
#include <boost/multiprecision/concepts/mp_number_archetypes.hpp>
#endif
#ifdef TEST_CPP_DEC_FLOAT
#include <boost/multiprecision/cpp_dec_float.hpp>
#endif
#if defined(TEST_MPFR) || defined(TEST_MPFR_50)
#include <boost/multiprecision/mpfr.hpp>
#endif
#if defined(TEST_MPFI_50)
#include <boost/multiprecision/mpfi.hpp>
#endif
#ifdef TEST_TOMMATH
#include <boost/multiprecision/tommath.hpp>
#endif
#ifdef TEST_CPP_INT
#include <boost/multiprecision/cpp_int.hpp>
#endif
#ifdef TEST_FLOAT128
#include <boost/multiprecision/float128.hpp>
#endif

#ifdef BOOST_MSVC
#pragma warning(disable:4127)
#endif

#define PRINT(x)\
   std::cout << BOOST_STRINGIZE(x) << " = " << std::numeric_limits<Number>::x << std::endl;

template <class Number>
void test_specific(const boost::mpl::int_<boost::multiprecision::number_kind_floating_point>&)
{
   Number minv, maxv;
   minv = (std::numeric_limits<Number>::min)();
   maxv = (std::numeric_limits<Number>::max)();
   BOOST_CHECK((boost::math::isnormal)(minv));
   BOOST_CHECK((boost::math::isnormal)(maxv));
   BOOST_CHECK((boost::math::isnormal)(log(minv)));
   BOOST_CHECK((boost::math::isnormal)(log(maxv)));
   BOOST_CHECK((boost::math::isnormal)(sqrt(minv)));
   BOOST_CHECK((boost::math::isnormal)(sqrt(maxv)));

   if(std::numeric_limits<Number>::is_specialized)
   {
      if(std::numeric_limits<Number>::has_quiet_NaN)
      {
         BOOST_TEST((boost::math::isnan)(std::numeric_limits<Number>::quiet_NaN()));
         BOOST_TEST(FP_NAN == (boost::math::fpclassify)(std::numeric_limits<Number>::quiet_NaN()));
         BOOST_TEST(!(boost::math::isfinite)(std::numeric_limits<Number>::quiet_NaN()));
         BOOST_TEST(!(boost::math::isnormal)(std::numeric_limits<Number>::quiet_NaN()));
         BOOST_TEST(!(boost::math::isinf)(std::numeric_limits<Number>::quiet_NaN()));
      }
      if(std::numeric_limits<Number>::has_signaling_NaN)
      {
         BOOST_TEST((boost::math::isnan)(std::numeric_limits<Number>::signaling_NaN()));
         BOOST_TEST(FP_NAN == (boost::math::fpclassify)(std::numeric_limits<Number>::signaling_NaN()));
         BOOST_TEST(!(boost::math::isfinite)(std::numeric_limits<Number>::signaling_NaN()));
         BOOST_TEST(!(boost::math::isnormal)(std::numeric_limits<Number>::signaling_NaN()));
         BOOST_TEST(!(boost::math::isinf)(std::numeric_limits<Number>::signaling_NaN()));
      }
      if(std::numeric_limits<Number>::has_infinity)
      {
         BOOST_TEST((boost::math::isinf)(std::numeric_limits<Number>::infinity()));
         BOOST_TEST(FP_INFINITE == (boost::math::fpclassify)(std::numeric_limits<Number>::infinity()));
         BOOST_TEST(!(boost::math::isfinite)(std::numeric_limits<Number>::infinity()));
         BOOST_TEST(!(boost::math::isnormal)(std::numeric_limits<Number>::infinity()));
         BOOST_TEST(!(boost::math::isnan)(std::numeric_limits<Number>::infinity()));
      }
      if(std::numeric_limits<Number>::has_denorm)
      {
         BOOST_TEST(FP_SUBNORMAL == (boost::math::fpclassify)(std::numeric_limits<Number>::denorm_min()));
         BOOST_TEST((boost::math::isfinite)(std::numeric_limits<Number>::denorm_min()));
         BOOST_TEST(!(boost::math::isnormal)(std::numeric_limits<Number>::denorm_min()));
         BOOST_TEST(!(boost::math::isinf)(std::numeric_limits<Number>::denorm_min()));
         BOOST_TEST(!(boost::math::isnan)(std::numeric_limits<Number>::denorm_min()));
      }
   }
   Number n = 0;
   BOOST_TEST((boost::math::fpclassify)(n) == FP_ZERO);
   BOOST_TEST((boost::math::isfinite)(n));
   BOOST_TEST(!(boost::math::isnormal)(n));
   BOOST_TEST(!(boost::math::isinf)(n));
   BOOST_TEST(!(boost::math::isnan)(n));
   n = 2;
   BOOST_TEST((boost::math::fpclassify)(n) == FP_NORMAL);
   BOOST_TEST((boost::math::isfinite)(n));
   BOOST_TEST((boost::math::isnormal)(n));
   BOOST_TEST(!(boost::math::isinf)(n));
   BOOST_TEST(!(boost::math::isnan)(n));
}

template <class Number>
void test_specific(const boost::mpl::int_<boost::multiprecision::number_kind_integer>&)
{
   if(std::numeric_limits<Number>::is_modulo)
   {
      if(!std::numeric_limits<Number>::is_signed)
      {
         BOOST_TEST(1 + (std::numeric_limits<Number>::max)() == 0);
         BOOST_TEST(--Number(0) == (std::numeric_limits<Number>::max)());
      }
   }
}

template <class Number, class T>
void test_specific(const T&)
{
}

template <class Number>
void test()
{
   //
   // Note really a test just yet, but we can at least print out all the values:
   //
   std::cout << "numeric_limits values for type " << typeid(Number).name() << std::endl;

   PRINT(is_specialized);
   if(std::numeric_limits<Number>::is_integer)
   {
      std::cout << std::hex << std::showbase;
   }
   std::cout << "max()" << " = " << (std::numeric_limits<Number>::max)() << std::endl;
   if(std::numeric_limits<Number>::is_integer)
   {
      std::cout << std::dec;
   }
   std::cout << "max()" << " = " << (std::numeric_limits<Number>::max)() << std::endl;
   std::cout << "min()" << " = " << (std::numeric_limits<Number>::min)() << std::endl;
#ifndef BOOST_NO_CXX11_NUMERIC_LIMITS
   PRINT(lowest());
#endif
   PRINT(digits);
   PRINT(digits10);
#ifndef BOOST_NO_CXX11_NUMERIC_LIMITS
   PRINT(max_digits10);
#endif
   PRINT(is_signed);
   PRINT(is_integer);
   PRINT(is_exact);
   PRINT(radix);
   PRINT(epsilon());
   PRINT(round_error());
   PRINT(min_exponent);
   PRINT(min_exponent10);
   PRINT(max_exponent);
   PRINT(max_exponent10);
   PRINT(has_infinity);
   PRINT(has_quiet_NaN);
   PRINT(has_signaling_NaN);
   PRINT(has_denorm);
   PRINT(has_denorm_loss);
   PRINT(infinity());
   PRINT(quiet_NaN());
   PRINT(signaling_NaN());
   PRINT(denorm_min());
   PRINT(is_iec559);
   PRINT(is_bounded);
   PRINT(is_modulo);
   PRINT(traps);
   PRINT(tinyness_before);
   PRINT(round_style);

   typedef typename boost::mpl::if_c<
      std::numeric_limits<Number>::is_specialized,
      typename boost::multiprecision::number_category<Number>::type,
      boost::mpl::int_<500> // not a number type
   >::type fp_test_type;

   test_specific<Number>(fp_test_type());
}


int main()
{
#ifdef TEST_BACKEND
   test<boost::multiprecision::number<boost::multiprecision::concepts::number_backend_float_architype> >();
#endif
#ifdef TEST_MPF_50
   test<boost::multiprecision::mpf_float_50>();
#endif
#ifdef TEST_MPF
   boost::multiprecision::mpf_float::default_precision(1000);
   /*
   boost::multiprecision::mpf_float r;
   r.precision(50);
   BOOST_TEST(r.precision() >= 50);
   */
   BOOST_TEST(boost::multiprecision::mpf_float::default_precision() == 1000);
   test<boost::multiprecision::mpf_float>();
#endif
#ifdef TEST_MPZ
   test<boost::multiprecision::mpz_int>();
#endif
#ifdef TEST_MPQ
   test<boost::multiprecision::mpq_rational>();
#endif
#ifdef TEST_CPP_DEC_FLOAT
   test<boost::multiprecision::cpp_dec_float_50>();
   test<boost::multiprecision::cpp_dec_float_100>();
#endif
#ifdef TEST_MPFR
   test<boost::multiprecision::mpfr_float>();
#endif
#ifdef TEST_MPFR_50
   test<boost::multiprecision::mpfr_float_50>();
#endif
#ifdef TEST_MPFI_50
   test<boost::multiprecision::mpfi_float_50>();
   test<boost::multiprecision::mpfi_float>();
#endif
#ifdef TEST_TOMMATH
   test<boost::multiprecision::tom_int>();
#endif
#ifdef TEST_CPP_INT
   test<boost::multiprecision::cpp_int>();
   test<boost::multiprecision::int256_t>();
   test<boost::multiprecision::uint512_t>();
   test<boost::multiprecision::number<boost::multiprecision::cpp_int_backend<200, 200, boost::multiprecision::unsigned_magnitude, boost::multiprecision::checked, void> > >();
   test<boost::multiprecision::number<boost::multiprecision::cpp_int_backend<70, 70, boost::multiprecision::signed_magnitude, boost::multiprecision::unchecked, void> > >();
#endif
#ifdef TEST_FLOAT128
   test<boost::multiprecision::float128>();
#endif
   return boost::report_errors();
}

