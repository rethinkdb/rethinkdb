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

template <class Number>
void test()
{
   using namespace boost::multiprecision;
   typedef Number test_type;

   if(std::numeric_limits<test_type>::is_bounded)
   {
      test_type val = (std::numeric_limits<test_type>::max)();
      BOOST_CHECK_THROW(++val, std::overflow_error);
      val = (std::numeric_limits<test_type>::max)();
      BOOST_CHECK_THROW(test_type(1 + val), std::overflow_error);
      BOOST_CHECK_THROW(test_type(val + 1), std::overflow_error);
      BOOST_CHECK_THROW(test_type(2 * val), std::overflow_error);
      val /= 2;
      val += 1;
      BOOST_CHECK_THROW(test_type(2 * val), std::overflow_error);

      if(std::numeric_limits<test_type>::is_signed)
      {
         val = (std::numeric_limits<test_type>::min)();
         BOOST_CHECK_THROW(--val, std::overflow_error);
         val = (std::numeric_limits<test_type>::min)();
         BOOST_CHECK_THROW(test_type(val - 1), std::overflow_error);
         BOOST_CHECK_THROW(test_type(2 * val), std::overflow_error);
         val /= 2;
         val -= 1;
         BOOST_CHECK_THROW(test_type(2 * val), std::overflow_error);
      }
      else
      {
         val = (std::numeric_limits<test_type>::min)();
         BOOST_CHECK_THROW(--val, std::range_error);
         val = (std::numeric_limits<test_type>::min)();
         BOOST_CHECK_THROW(test_type(val - 1), std::range_error);
      }
   }

   if(std::numeric_limits<test_type>::is_signed)
   {
      test_type a = -1;
      test_type b = 1;
      BOOST_CHECK_THROW(test_type(a | b), std::range_error);
      BOOST_CHECK_THROW(test_type(a & b), std::range_error);
      BOOST_CHECK_THROW(test_type(a ^ b), std::range_error);
   }
   else
   {
      // Constructing from a negative value is not allowed:
      BOOST_CHECK_THROW(test_type(-2), std::range_error);
      BOOST_CHECK_THROW(test_type("-2"), std::range_error);
   }
   if(std::numeric_limits<test_type>::digits < std::numeric_limits<long long>::digits)
   {
      long long llm = (std::numeric_limits<long long>::max)();
      test_type t;
      BOOST_CHECK_THROW(t = llm, std::range_error);
      BOOST_CHECK_THROW(t = static_cast<test_type>(llm), std::range_error);
      unsigned long long ullm = (std::numeric_limits<unsigned long long>::max)();
      BOOST_CHECK_THROW(t = ullm, std::range_error);
      BOOST_CHECK_THROW(t = static_cast<test_type>(ullm), std::range_error);

      static const checked_uint512_t big = (std::numeric_limits<checked_uint512_t>::max)();
      BOOST_CHECK_THROW(t = static_cast<test_type>(big), std::range_error);
   }
   //
   // String errors:
   //
   BOOST_CHECK_THROW(test_type("12A"), std::runtime_error);
   BOOST_CHECK_THROW(test_type("0658"), std::runtime_error);

   if(std::numeric_limits<test_type>::is_signed)
   {
      BOOST_CHECK_THROW(test_type(-2).str(0, std::ios_base::hex), std::runtime_error);
      BOOST_CHECK_THROW(test_type(-2).str(0, std::ios_base::oct), std::runtime_error);
   }
}

int main()
{
   using namespace boost::multiprecision;

   test<number<cpp_int_backend<0, 0, signed_magnitude, checked> > >();
   test<checked_int512_t>();
   test<checked_uint512_t>();
   test<number<cpp_int_backend<32, 32, signed_magnitude, checked, void> > >();
   test<number<cpp_int_backend<32, 32, unsigned_magnitude, checked, void> > >();

   //
   // We also need to test type with "odd" bit counts in order to ensure full code coverage:
   //
   test<number<cpp_int_backend<528, 528, signed_magnitude, checked, void> > >();
   test<number<cpp_int_backend<528, 528, unsigned_magnitude, checked, void> > >();
   test<number<cpp_int_backend<48, 48, signed_magnitude, checked, void> > >();
   test<number<cpp_int_backend<48, 48, unsigned_magnitude, checked, void> > >();
   return boost::report_errors();
}



