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

#include <boost/multiprecision/cpp_int.hpp>
#include "test.hpp"

template <class Number, class BigNumber>
void test()
{
   using namespace boost::multiprecision;
   typedef Number test_type;

   test_type h = (std::numeric_limits<test_type>::max)();
   test_type l = (std::numeric_limits<test_type>::max)();
   BigNumber r;

   add(r, h, h);
   BOOST_CHECK_EQUAL(r, cpp_int(h) + cpp_int(h));

   multiply(r, h, h);
   BOOST_CHECK_EQUAL(r, cpp_int(h) * cpp_int(h));

   if(std::numeric_limits<test_type>::is_signed)
   {
      subtract(r, l, h);
      BOOST_CHECK_EQUAL(r, cpp_int(l) - cpp_int(h));
      subtract(r, h, l);
      BOOST_CHECK_EQUAL(r, cpp_int(h) - cpp_int(l));
      multiply(r, l, l);
      BOOST_CHECK_EQUAL(r, cpp_int(l) * cpp_int(l));
   }

   //
   // Try again with integer types as the source:
   //
   enum{ max_digits = std::numeric_limits<test_type>::is_signed ? std::numeric_limits<long long>::digits : std::numeric_limits<unsigned long long>::digits };
   enum{ require_digits = std::numeric_limits<test_type>::digits <= 2 * max_digits ? std::numeric_limits<test_type>::digits / 2 : max_digits };
   typedef typename boost::uint_t<require_digits>::least uint_least;
   typedef typename boost::int_t<require_digits>::least  int_least;
   typedef typename boost::mpl::if_c<std::numeric_limits<test_type>::is_signed, int_least, uint_least>::type i_type;

   i_type ih = (std::numeric_limits<i_type>::max)();
   i_type il = (std::numeric_limits<i_type>::max)();

   add(r, ih, ih);
   BOOST_CHECK_EQUAL(r, cpp_int(ih) + cpp_int(ih));

   multiply(r, ih, ih);
   BOOST_CHECK_EQUAL(r, cpp_int(ih) * cpp_int(ih));

   if(std::numeric_limits<test_type>::is_signed)
   {
      subtract(r, il, ih);
      BOOST_CHECK_EQUAL(r, cpp_int(il) - cpp_int(ih));
      subtract(r, ih, il);
      BOOST_CHECK_EQUAL(r, cpp_int(ih) - cpp_int(il));
      multiply(r, il, il);
      BOOST_CHECK_EQUAL(r, cpp_int(il) * cpp_int(il));
   }
}

void test_rational_mixed()
{
   using namespace boost::multiprecision;
   cpp_int a(2);
   cpp_rational r(10);

   BOOST_CHECK_EQUAL(a + -r, -8);
   BOOST_CHECK_EQUAL(-r + a, -8);
   BOOST_CHECK_EQUAL(-a + r, 8);
   BOOST_CHECK_EQUAL(r + -a, 8);

   BOOST_CHECK_EQUAL(a - -r, 12);
   BOOST_CHECK_EQUAL(-r - a, -12);
   BOOST_CHECK_EQUAL(-a - r, -12);
   BOOST_CHECK_EQUAL(r - -a, 12);

   BOOST_CHECK_EQUAL(a * -r, -20);
   BOOST_CHECK_EQUAL(-r * a, -20);
   BOOST_CHECK_EQUAL(-a * r, -20);
   BOOST_CHECK_EQUAL(r * -a, -20);

   BOOST_CHECK_EQUAL(a / -r, cpp_rational(-2, 10));
   BOOST_CHECK_EQUAL(-r / a, -5);
   BOOST_CHECK_EQUAL(cpp_rational(-a / r), cpp_rational(-2, 10));
   BOOST_CHECK_EQUAL(r / -a, -5);
}

int main()
{
   using namespace boost::multiprecision;

   test_rational_mixed();

   test<checked_int512_t, checked_int1024_t>();
   test<checked_int256_t, checked_int512_t>();
   test<number<cpp_int_backend<64, 64, signed_magnitude, checked, void>, et_off>, checked_int128_t>();
   test<boost::int64_t, checked_int128_t>();

   test<checked_uint512_t, checked_uint1024_t>();
   test<checked_uint256_t, checked_uint512_t>();
   test<number<cpp_int_backend<64, 64, unsigned_magnitude, checked, void>, et_off>, checked_uint128_t>();
   test<boost::uint64_t, checked_int128_t>();

   return boost::report_errors();
}



