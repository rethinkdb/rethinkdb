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

#ifdef TEST_GMP
#include <boost/multiprecision/gmp.hpp>
#endif
#ifdef TEST_MPFR
#include <boost/multiprecision/mpfr.hpp>
#endif
#include <boost/multiprecision/cpp_dec_float.hpp>
#include "test.hpp"

template <class Number, class BigNumber>
void test()
{
   using namespace boost::multiprecision;
   typedef Number test_type;

   test_type a = 1;
   a /= 3;
   test_type b = -a;

   BigNumber r;
   BOOST_CHECK_EQUAL(add(r, a, a), BigNumber(a) + BigNumber(a));
   BOOST_CHECK_EQUAL(subtract(r, a, b), BigNumber(a) - BigNumber(b));
   BOOST_CHECK_EQUAL(subtract(r, b, a), BigNumber(b) - BigNumber(a));
   BOOST_CHECK_EQUAL(multiply(r, a, a), BigNumber(a) * BigNumber(a));
}

int main()
{
   using namespace boost::multiprecision;

   test<cpp_dec_float_50, cpp_dec_float_100>();

#ifdef TEST_GMP
   test<mpf_float_50, mpf_float_100>();
#endif
#ifdef TEST_MPFR
   test<mpfr_float_50, mpfr_float_100>();
#endif

   return boost::report_errors();
}



