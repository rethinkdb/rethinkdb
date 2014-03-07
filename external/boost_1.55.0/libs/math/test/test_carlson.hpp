// Copyright John Maddock 2006.
// Copyright Paul A. Bristow 2007, 2009
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/concepts/real_concept.hpp>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/array.hpp>
#include <boost/tr1/random.hpp>
#include "functor.hpp"

#include "handle_test_result.hpp"
#include "table_type.hpp"

#ifndef SC_
#define SC_(x) static_cast<typename table_type<T>::type>(BOOST_JOIN(x, L))
#endif

template <class Real, typename T>
void do_test_ellint_rf(T& data, const char* type_name, const char* test)
{
   typedef Real                   value_type;

   std::cout << "Testing: " << test << std::endl;

#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
    value_type (*fp)(value_type, value_type, value_type) = boost::math::ellint_rf<value_type, value_type, value_type>;
#else
    value_type (*fp)(value_type, value_type, value_type) = boost::math::ellint_rf;
#endif
    boost::math::tools::test_result<value_type> result;
 
    result = boost::math::tools::test_hetero<Real>(
      data, 
      bind_func<Real>(fp, 0, 1, 2),
      extract_result<Real>(3));
   handle_test_result(result, data[result.worst()], result.worst(), 
      type_name, "boost::math::ellint_rf", test);

   std::cout << std::endl;

}

template <class Real, typename T>
void do_test_ellint_rc(T& data, const char* type_name, const char* test)
{
   typedef Real                   value_type;

   std::cout << "Testing: " << test << std::endl;

#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
    value_type (*fp)(value_type, value_type) = boost::math::ellint_rc<value_type, value_type>;
#else
    value_type (*fp)(value_type, value_type) = boost::math::ellint_rc;
#endif
    boost::math::tools::test_result<value_type> result;
 
    result = boost::math::tools::test_hetero<Real>(
      data, 
      bind_func<Real>(fp, 0, 1),
      extract_result<Real>(2));
      handle_test_result(result, data[result.worst()], result.worst(), 
      type_name, "boost::math::ellint_rc", test);

   std::cout << std::endl;

}

template <class Real, typename T>
void do_test_ellint_rj(T& data, const char* type_name, const char* test)
{
   typedef Real                   value_type;

   std::cout << "Testing: " << test << std::endl;

#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
    value_type (*fp)(value_type, value_type, value_type, value_type) = boost::math::ellint_rj<value_type, value_type, value_type, value_type>;
#else
    value_type (*fp)(value_type, value_type, value_type, value_type) = boost::math::ellint_rj;
#endif
    boost::math::tools::test_result<value_type> result;
 
    result = boost::math::tools::test_hetero<Real>(
      data, 
      bind_func<Real>(fp, 0, 1, 2, 3),
      extract_result<Real>(4));
      handle_test_result(result, data[result.worst()], result.worst(), 
      type_name, "boost::math::ellint_rf", test);

   std::cout << std::endl;

}

template <class Real, typename T>
void do_test_ellint_rd(T& data, const char* type_name, const char* test)
{
   typedef Real                   value_type;

   std::cout << "Testing: " << test << std::endl;

#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
    value_type (*fp)(value_type, value_type, value_type) = boost::math::ellint_rd<value_type, value_type, value_type>;
#else
    value_type (*fp)(value_type, value_type, value_type) = boost::math::ellint_rd;
#endif
    boost::math::tools::test_result<value_type> result;
 
    result = boost::math::tools::test_hetero<Real>(
      data, 
      bind_func<Real>(fp, 0, 1, 2),
      extract_result<Real>(3));
    handle_test_result(result, data[result.worst()], result.worst(), 
      type_name, "boost::math::ellint_rd", test);

   std::cout << std::endl;

}

template <typename T>
void test_spots(T, const char* type_name)
{
#ifndef TEST_UDT
   using namespace boost::math;
   using namespace std;
   // Spot values from Numerical Computation of Real or Complex 
   // Elliptic Integrals, B. C. Carlson: http://arxiv.org/abs/math.CA/9409227
   // RF:
   T tolerance = (std::max)(T(1e-13f), tools::epsilon<T>() * 5) * 100; // Note 5eps expressed as a persentage!!!
   T eps2 = 2 * tools::epsilon<T>();
   BOOST_CHECK_CLOSE(ellint_rf(T(1), T(2), T(0)), T(1.3110287771461), tolerance);
   BOOST_CHECK_CLOSE(ellint_rf(T(0.5), T(1), T(0)), T(1.8540746773014), tolerance);
   BOOST_CHECK_CLOSE(ellint_rf(T(2), T(3), T(4)), T(0.58408284167715), tolerance);
   // RC:
   BOOST_CHECK_CLOSE_FRACTION(ellint_rc(T(0), T(1)/4), boost::math::constants::pi<T>(), eps2);
   BOOST_CHECK_CLOSE_FRACTION(ellint_rc(T(9)/4, T(2)), log(T(2)), eps2);
   BOOST_CHECK_CLOSE_FRACTION(ellint_rc(T(1)/4, T(-2)), log(T(2))/3, eps2);
   // RJ:
   BOOST_CHECK_CLOSE(ellint_rj(T(0), T(1), T(2), T(3)), T(0.77688623778582), tolerance);
   BOOST_CHECK_CLOSE(ellint_rj(T(2), T(3), T(4), T(5)), T(0.14297579667157), tolerance);
   BOOST_CHECK_CLOSE(ellint_rj(T(2), T(3), T(4), T(-0.5)), T(0.24723819703052), tolerance);
   BOOST_CHECK_CLOSE(ellint_rj(T(2), T(3), T(4), T(-5)), T(-0.12711230042964), tolerance);
   // RD:
   BOOST_CHECK_CLOSE(ellint_rd(T(0), T(2), T(1)), T(1.7972103521034), tolerance);
   BOOST_CHECK_CLOSE(ellint_rd(T(2), T(3), T(4)), T(0.16510527294261), tolerance);

   // Sanity/consistency checks from Numerical Computation of Real or Complex 
   // Elliptic Integrals, B. C. Carlson: http://arxiv.org/abs/math.CA/9409227
   std::tr1::mt19937 ran;
   std::tr1::uniform_real<float> ur(0, 1000);
   T eps40 = 40 * tools::epsilon<T>();

   for(unsigned i = 0; i < 1000; ++i)
   {
      T x = ur(ran);
      T y = ur(ran);
      T z = ur(ran);
      T lambda = ur(ran);
      T mu = x * y / lambda;
      // RF, eq 49:
      T s1 = ellint_rf(x+lambda, y+lambda, lambda) + 
         ellint_rf(x + mu, y + mu, mu);
      T s2 = ellint_rf(x, y, T(0));
      BOOST_CHECK_CLOSE_FRACTION(s1, s2, eps40);
      // RC is degenerate case of RF:
      s1 = ellint_rc(x, y);
      s2 = ellint_rf(x, y, y);
      BOOST_CHECK_CLOSE_FRACTION(s1, s2, eps40);
      // RC, eq 50 (Note have to assume y = x):
      T mu2 = x * x / lambda;
      s1 = ellint_rc(lambda, x+lambda) 
         + ellint_rc(mu2, x + mu2);
      s2 = ellint_rc(T(0), x);
      BOOST_CHECK_CLOSE_FRACTION(s1, s2, eps40);
      /*
      T p = ????; // no closed form for a, b and p???
      s1 = ellint_rj(x+lambda, y+lambda, lambda, p+lambda)
         + ellint_rj(x+mu, y+mu, mu, p+mu);
      s2 = ellint_rj(x, y, T(0), p)
         - 3 * ellint_rc(a, b);
      */
      // RD, eq 53:
      s1 = ellint_rd(lambda, x+lambda, y+lambda)
         + ellint_rd(mu, x+mu, y+mu);
      s2 = ellint_rd(T(0), x, y)
         - 3 / (y * sqrt(x+y+lambda+mu));
      BOOST_CHECK_CLOSE_FRACTION(s1, s2, eps40);
      // RD is degenerate case of RJ:
      s1 = ellint_rd(x, y, z);
      s2 = ellint_rj(x, y, z, z);
      BOOST_CHECK_CLOSE_FRACTION(s1, s2, eps40);
   }
#endif
   //
   // Now random spot values:
   //
#include "ellint_rf_data.ipp"

   do_test_ellint_rf<T>(ellint_rf_data, type_name, "RF: Random data");

#include "ellint_rc_data.ipp"

   do_test_ellint_rc<T>(ellint_rc_data, type_name, "RC: Random data");

#include "ellint_rj_data.ipp"

   do_test_ellint_rj<T>(ellint_rj_data, type_name, "RJ: Random data");

#include "ellint_rd_data.ipp"

   do_test_ellint_rd<T>(ellint_rd_data, type_name, "RD: Random data");
}

