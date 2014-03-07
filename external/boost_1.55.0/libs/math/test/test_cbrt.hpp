// Copyright John Maddock 2006.
// Copyright Paul A. Bristow 2007, 2009
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/concepts/real_concept.hpp>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/math/tools/stats.hpp>
#include <boost/math/tools/test.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/array.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include "functor.hpp"

#include "handle_test_result.hpp"
#include "table_type.hpp"

#ifndef SC_
#define SC_(x) static_cast<typename table_type<T>::type>(BOOST_JOIN(x, L))
#endif

template <class Real>
struct negative_cbrt
{
   negative_cbrt(){}

   template <class S>
   Real operator()(const S& row)
   {
      return boost::math::cbrt(Real(-Real(row[1])));
   }
};


template <class Real, class T>
void do_test_cbrt(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = boost::math::cbrt<value_type>;
#else
   pg funcp = boost::math::cbrt;
#endif

   boost::math::tools::test_result<value_type> result;

   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

   //
   // test cbrt against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data, 
      bind_func<Real>(funcp, 1), 
      extract_result<Real>(0));
   result += boost::math::tools::test_hetero<Real>(
      data, 
      negative_cbrt<Real>(), 
      negate<Real>(extract_result<Real>(0)));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::cbrt", test_name);
   std::cout << std::endl;
}
template <class T>
void test_cbrt(T, const char* name)
{
   //
   // The actual test data is rather verbose, so it's in a separate file.
   //
   // The contents are as follows, each row of data contains
   // three items, input value a, input value b and erf(a, b):
   // 
#  include "cbrt_data.ipp"

   do_test_cbrt<T>(cbrt_data, name, "cbrt Function");

}

