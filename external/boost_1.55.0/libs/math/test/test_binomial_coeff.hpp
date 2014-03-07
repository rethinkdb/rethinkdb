// Copyright John Maddock 2006.
// Copyright Paul A. Bristow 2007, 2009
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/concepts/real_concept.hpp>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/math/special_functions/binomial.hpp>
#include <boost/math/special_functions/trunc.hpp>
#include <boost/math/tools/test.hpp>
#include "functor.hpp"
#include <boost/array.hpp>

#include "handle_test_result.hpp"
#include "table_type.hpp"

#ifndef SC_
#define SC_(x) static_cast<typename table_type<T>::type>(BOOST_JOIN(x, L))
#endif

template <class T>
T binomial_wrapper(T n, T k)
{
   return boost::math::binomial_coefficient<T>(
      boost::math::itrunc(n),
      boost::math::itrunc(k));
}

template <class T>
void test_binomial(T, const char* type_name)
{
   using namespace std;

   typedef T (*func_t)(T, T);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   func_t f = &binomial_wrapper<T>;
#else
   func_t f = &binomial_wrapper;
#endif

#include "binomial_data.ipp"

   boost::math::tools::test_result<T> result = boost::math::tools::test_hetero<T>(
      binomial_data, 
      bind_func<T>(f, 0, 1), 
      extract_result<T>(2));

   std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
      "Test results for small arguments and type " << type_name << std::endl << std::endl;
   std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
   handle_test_result(result, binomial_data[result.worst()], result.worst(), type_name, "binomial_coefficient", "Binomials: small arguments");
   std::cout << std::endl;

#include "binomial_large_data.ipp"

   result = boost::math::tools::test_hetero<T>(
      binomial_large_data, 
      bind_func<T>(f, 0, 1), 
      extract_result<T>(2));

   std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
      "Test results for large arguments and type " << type_name << std::endl << std::endl;
   std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
   handle_test_result(result, binomial_large_data[result.worst()], result.worst(), type_name, "binomial_coefficient", "Binomials: large arguments");
   std::cout << std::endl;
}

