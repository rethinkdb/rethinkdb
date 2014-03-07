//  Copyright John Maddock 2009.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "required_defines.hpp"

#include "performance_measure.hpp"

#include <boost/math/special_functions/beta.hpp>
#include <boost/array.hpp>

#define T double
#  include "../test/beta_small_data.ipp"
#  include "../test/beta_med_data.ipp"
#  include "../test/beta_exp_data.ipp"

template <std::size_t N>
double beta_evaluate2(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += boost::math::beta(data[i][0], data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(beta_test, "beta")
{
   double result = beta_evaluate2(beta_small_data);
   result += beta_evaluate2(beta_med_data);
   result += beta_evaluate2(beta_exp_data);

   consume_result(result);
   set_call_count(
      (sizeof(beta_small_data) 
      + sizeof(beta_med_data) 
      + sizeof(beta_exp_data)) / sizeof(beta_exp_data[0]));
}

#ifdef TEST_DCDFLIB
#include <dcdflib.h>

template <std::size_t N>
double beta_evaluate2_dcd(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += ::beta(data[i][0], data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(beta_test_dcd, "beta-dcd")
{
   double result = beta_evaluate2_dcd(beta_small_data);
   result += beta_evaluate2_dcd(beta_med_data);
   result += beta_evaluate2_dcd(beta_exp_data);

   consume_result(result);
   set_call_count(
      (sizeof(beta_small_data) 
      + sizeof(beta_med_data) 
      + sizeof(beta_exp_data)) / sizeof(beta_exp_data[0]));
}

#endif


