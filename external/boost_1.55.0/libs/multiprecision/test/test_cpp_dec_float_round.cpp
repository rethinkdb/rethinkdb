///////////////////////////////////////////////////////////////
//  Copyright 2013 Christopher Kormanyos. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_
//

// Test case for ticket:
// #8065: Multiprecision rounding issue

#ifdef _MSC_VER
#  define _SCL_SECURE_NO_WARNINGS
#endif

#include <boost/detail/lightweight_test.hpp>
#include "test.hpp"
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/math/special_functions/round.hpp>

template<int N>
static bool round_test_imp()
{
  typedef boost::multiprecision::cpp_dec_float<N> mp_backend_type;
  typedef boost::multiprecision::number<mp_backend_type, boost::multiprecision::et_off> mp_type;

  const mp_type original_digits(1.0F);

  const mp_type scale = pow(mp_type(10), N);

  mp_type these_digits = original_digits * scale;

  these_digits  = boost::math::round(these_digits);
  these_digits /= scale;

  const std::string result = these_digits.str();

  return (result == original_digits.str());
}

template<int N>
struct round_test
{
  static bool test()
  {
    return (round_test_imp<N>() && round_test<N - 1>::test());
  }
};

template<>
struct round_test<0>
{
  static bool test()
  {
    return round_test_imp<0>();
  }
};

int main()
{
   //
   // Test cpp_dec_float rounding with boost::math::round() at various precisions:
   //
   const bool round_test_result = round_test<40>::test();

   BOOST_CHECK_EQUAL(round_test_result, true);

   return boost::report_errors();
}
