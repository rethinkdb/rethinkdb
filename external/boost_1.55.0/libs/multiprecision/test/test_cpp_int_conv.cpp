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

#include <boost/multiprecision/cpp_int.hpp>

int main()
{
   using namespace boost::multiprecision;
   //
   // Test interconversions between different precisions:
   //
   cpp_int    i1(2);
   int128_t   i2(3);
   int256_t   i3(4);
   number<cpp_int_backend<32, 32, signed_magnitude, checked, void> > i4(5);

   i1 = i3;
   BOOST_TEST(i1 == 4);
   i1 = i4;
   BOOST_TEST(i1 == 5);

   i3 = -1234567;
   i4 = -5677334;
   i1 = i3;
   BOOST_TEST(i1 == -1234567);
   i1 = i4;
   BOOST_TEST(i1 == -5677334);

   i3 = i2;
   BOOST_TEST(i3 == 3);

   i3 = -1234567;

   uint128_t  i5(i3);
   BOOST_TEST(i5 == -1234567);

   int128_t   i6(i4);
   BOOST_TEST(i6 == -5677334);

   number<cpp_int_backend<32, 32, signed_magnitude, unchecked, void>, et_off> i7(i3);
   BOOST_TEST(i7 == -1234567);

   int256_t   i8(i6);
   BOOST_TEST(i8 == -5677334);

   i7.assign(4.0);
   BOOST_TEST(i7 == 4);

   number<cpp_int_backend<30, 30, signed_magnitude, checked, void>, et_off> i9(-5677334);
   i7 = i9;
   BOOST_TEST(i7 == -5677334);
   i7 = number<cpp_int_backend<32, 32, signed_magnitude, checked, void>, et_off>(i9);
   BOOST_TEST(i7 == -5677334);

   i9 = static_cast<number<cpp_int_backend<30, 30, signed_magnitude, checked, void>, et_off> >(i7);
   BOOST_TEST(i9 == -5677334);

   ++i9;
   i7 = i9;
   BOOST_TEST(i7 == 1 - 5677334);

   return boost::report_errors();
}



