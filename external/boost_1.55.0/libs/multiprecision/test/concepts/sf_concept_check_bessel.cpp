//  Copyright John Maddock 2012.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//
// This tests that cpp_dec_float_50 meets our
// conceptual requirements when used with Boost.Math.
//
#ifdef _MSC_VER
#  define _SCL_SECURE_NO_WARNINGS
#  pragma warning(disable:4800)
#  pragma warning(disable:4512)
#  pragma warning(disable:4127)
#  pragma warning(disable:4512)
#  pragma warning(disable:4503) // decorated name length exceeded, name was truncated
#endif

#include <libs/math/test/compile_test/poison.hpp>

#if !defined(TEST_MPF_50) && !defined(TEST_BACKEND) && !defined(TEST_MPZ) \
   && !defined(TEST_CPP_DEC_FLOAT) && !defined(TEST_MPFR_50)\
   && !defined(TEST_MPFR_6) && !defined(TEST_MPFR_15) && !defined(TEST_MPFR_17) \
   && !defined(TEST_MPFR_30) && !defined(TEST_CPP_DEC_FLOAT_NO_ET) && !defined(TEST_LOGGED_ADAPTER)
#  define TEST_MPF_50
#  define TEST_BACKEND
#  define TEST_MPZ
#  define TEST_MPFR_50
#  define TEST_MPFR_6
#  define TEST_MPFR_15
#  define TEST_MPFR_17
#  define TEST_MPFR_30
#  define TEST_CPP_DEC_FLOAT
#  define TEST_CPP_DEC_FLOAT_NO_ET
#  define TEST_LOGGED_ADAPTER

#ifdef _MSC_VER
#pragma message("CAUTION!!: No backend type specified so testing everything.... this will take some time!!")
#endif
#ifdef __GNUC__
#pragma warning "CAUTION!!: No backend type specified so testing everything.... this will take some time!!"
#endif

#endif

#if defined(TEST_MPF_50) || defined(TEST_MPZ)
#include <boost/multiprecision/gmp.hpp>
#endif
#ifdef TEST_BACKEND
#include <boost/multiprecision/concepts/mp_number_archetypes.hpp>
#endif
#if defined(TEST_CPP_DEC_FLOAT) || defined(TEST_CPP_DEC_FLOAT_NO_ET) || defined(TEST_LOGGED_ADAPTER)
#include <boost/multiprecision/cpp_dec_float.hpp>
#endif
#if defined(TEST_MPFR_50) || defined(TEST_MPFR_6) || defined(TEST_MPFR_15) || defined(TEST_MPFR_17) || defined(TEST_MPFR_30)
#include <boost/multiprecision/mpfr.hpp>
#endif
#ifdef TEST_LOGGED_ADAPTER
#  include <boost/multiprecision/logged_adaptor.hpp>
#endif

#include <boost/math/special_functions.hpp>

template <class T>
void test_extra(T)
{
   T v1, v2, v3;
   int i(0);
   boost::math::cyl_neumann(v1, v2);
   boost::math::cyl_neumann(i, v2);
#ifndef SLOW_COMPILER
   boost::math::cyl_bessel_j(v1, v2);
   boost::math::cyl_bessel_j(i, v2);
   boost::math::cyl_bessel_i(v1, v2);
   boost::math::cyl_bessel_i(i, v2);
   boost::math::cyl_bessel_k(v1, v2);
   boost::math::cyl_bessel_k(i, v2);
   boost::math::sph_bessel(i, v2);
   boost::math::sph_bessel(i, 1);
   boost::math::sph_neumann(i, v2);
   boost::math::sph_neumann(i, i);
   boost::math::airy_ai(v1);
   boost::math::airy_bi(v1);
   boost::math::airy_ai_prime(v1);
   boost::math::airy_bi_prime(v1);
#endif
}

void foo()
{
#ifdef TEST_BACKEND
   test_extra(boost::multiprecision::concepts::mp_number_float_architype());
#endif
#ifdef TEST_MPF_50
   test_extra(boost::multiprecision::mpf_float_50());
#endif
#ifdef TEST_MPFR_50
   test_extra(boost::multiprecision::mpfr_float_50());
#endif
#ifdef TEST_MPFR_6
   test_extra(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<6> >());
#endif
#ifdef TEST_MPFR_15
   test_extra(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<15> >());
#endif
#ifdef TEST_MPFR_17
   test_extra(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<17> >());
#endif
#ifdef TEST_MPFR_30
   test_extra(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<30> >());
#endif
#ifdef TEST_CPP_DEC_FLOAT
   test_extra(boost::multiprecision::cpp_dec_float_50());
#endif
#ifdef TEST_CPP_DEC_FLOAT_NO_ET
   test_extra(boost::multiprecision::number<boost::multiprecision::cpp_dec_float<100>, boost::multiprecision::et_off>());
#endif
#ifdef TEST_LOGGED_ADAPTER
   typedef boost::multiprecision::number<boost::multiprecision::logged_adaptor<boost::multiprecision::cpp_dec_float<50> > > num_t;
   test_extra(num_t());
#endif
}

int main()
{
   foo();
}
