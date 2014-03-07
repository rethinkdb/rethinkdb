//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pch.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/results_collector.hpp>
#include <boost/math/special_functions/beta.hpp>
#include <boost/math/tools/roots.hpp>
#include <boost/test/results_collector.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/array.hpp>
#include "table_type.hpp"

#define BOOST_CHECK_CLOSE_EX(a, b, prec, i) \
   {\
      unsigned int failures = boost::unit_test::results_collector.results( boost::unit_test::framework::current_test_case().p_id ).p_assertions_failed;\
      BOOST_CHECK_CLOSE(a, b, prec); \
      if(failures != boost::unit_test::results_collector.results( boost::unit_test::framework::current_test_case().p_id ).p_assertions_failed)\
      {\
         std::cerr << "Failure was at row " << i << std::endl;\
         std::cerr << std::setprecision(35); \
         std::cerr << "{ " << data[i][0] << " , " << data[i][1] << " , " << data[i][2];\
         std::cerr << " , " << data[i][3] << " , " << data[i][4] << " , " << data[i][5] << " } " << std::endl;\
      }\
   }


//
// Implement various versions of inverse of the incomplete beta
// using different root finding algorithms, and deliberately "bad"
// starting conditions: that way we get all the pathological cases
// we could ever wish for!!!
//

template <class T, class Policy>
struct ibeta_roots_1   // for first order algorithms
{
   ibeta_roots_1(T _a, T _b, T t, bool inv = false)
      : a(_a), b(_b), target(t), invert(inv) {}

   T operator()(const T& x)
   {
      return boost::math::detail::ibeta_imp(a, b, x, Policy(), invert, true) - target;
   }
private:
   T a, b, target;
   bool invert;
};

template <class T, class Policy>
struct ibeta_roots_2   // for second order algorithms
{
   ibeta_roots_2(T _a, T _b, T t, bool inv = false)
      : a(_a), b(_b), target(t), invert(inv) {}

   boost::math::tuple<T, T> operator()(const T& x)
   {
      typedef typename boost::math::lanczos::lanczos<T, Policy>::type L;
      T f = boost::math::detail::ibeta_imp(a, b, x, Policy(), invert, true) - target;
      T f1 = invert ?
         -boost::math::detail::ibeta_power_terms(b, a, 1 - x, x, L(), true, Policy())
               : boost::math::detail::ibeta_power_terms(a, b, x, 1 - x, L(), true, Policy());
      T y = 1 - x;
      if(y == 0)
         y = boost::math::tools::min_value<T>() * 8;
      f1 /= y * x;

      // make sure we don't have a zero derivative:
      if(f1 == 0)
         f1 = (invert ? -1 : 1) * boost::math::tools::min_value<T>() * 64;

      return boost::math::make_tuple(f, f1);
   }
private:
   T a, b, target;
   bool invert;
};

template <class T, class Policy>
struct ibeta_roots_3   // for third order algorithms
{
   ibeta_roots_3(T _a, T _b, T t, bool inv = false)
      : a(_a), b(_b), target(t), invert(inv) {}

   boost::math::tuple<T, T, T> operator()(const T& x)
   {
      typedef typename boost::math::lanczos::lanczos<T, Policy>::type L;
      T f = boost::math::detail::ibeta_imp(a, b, x, Policy(), invert, true) - target;
      T f1 = invert ?
               -boost::math::detail::ibeta_power_terms(b, a, 1 - x, x, L(), true, Policy())
               : boost::math::detail::ibeta_power_terms(a, b, x, 1 - x, L(), true, Policy());
      T y = 1 - x;
      if(y == 0)
         y = boost::math::tools::min_value<T>() * 8;
      f1 /= y * x;
      T f2 = f1 * (-y * a + (b - 2) * x + 1) / (y * x);
      if(invert)
         f2 = -f2;

      // make sure we don't have a zero derivative:
      if(f1 == 0)
         f1 = (invert ? -1 : 1) * boost::math::tools::min_value<T>() * 64;

      return boost::math::make_tuple(f, f1, f2);
   }
private:
   T a, b, target;
   bool invert;
};

double inverse_ibeta_bisect(double a, double b, double z)
{
   typedef boost::math::policies::policy<> pol;
   bool invert = false;
   int bits = std::numeric_limits<double>::digits;

   //
   // special cases, we need to have these because there may be other
   // possible answers:
   //
   if(z == 1) return 1;
   if(z == 0) return 0;

   //
   // We need a good estimate of the error in the incomplete beta function
   // so that we don't set the desired precision too high.  Assume that 3-bits
   // are lost each time the arguments increase by a factor of 10:
   //
   using namespace std;
   int bits_lost = static_cast<int>(ceil(log10((std::max)(a, b)) * 3));
   if(bits_lost < 0)
      bits_lost = 3;
   else
      bits_lost += 3;
   int precision = bits - bits_lost;

   double min = 0;
   double max = 1;
   boost::math::tools::eps_tolerance<double> tol(precision);
   return boost::math::tools::bisect(ibeta_roots_1<double, pol>(a, b, z, invert), min, max, tol).first;
}

double inverse_ibeta_newton(double a, double b, double z)
{
   double guess = 0.5;
   bool invert = false;
   int bits = std::numeric_limits<double>::digits;

   //
   // special cases, we need to have these because there may be other
   // possible answers:
   //
   if(z == 1) return 1;
   if(z == 0) return 0;

   //
   // We need a good estimate of the error in the incomplete beta function
   // so that we don't set the desired precision too high.  Assume that 3-bits
   // are lost each time the arguments increase by a factor of 10:
   //
   using namespace std;
   int bits_lost = static_cast<int>(ceil(log10((std::max)(a, b)) * 3));
   if(bits_lost < 0)
      bits_lost = 3;
   else
      bits_lost += 3;
   int precision = bits - bits_lost;

   double min = 0;
   double max = 1;
   return boost::math::tools::newton_raphson_iterate(ibeta_roots_2<double, boost::math::policies::policy<> >(a, b, z, invert), guess, min, max, precision);
}

double inverse_ibeta_halley(double a, double b, double z)
{
   double guess = 0.5;
   bool invert = false;
   int bits = std::numeric_limits<double>::digits;

   //
   // special cases, we need to have these because there may be other
   // possible answers:
   //
   if(z == 1) return 1;
   if(z == 0) return 0;

   //
   // We need a good estimate of the error in the incomplete beta function
   // so that we don't set the desired precision too high.  Assume that 3-bits
   // are lost each time the arguments increase by a factor of 10:
   //
   using namespace std;
   int bits_lost = static_cast<int>(ceil(log10((std::max)(a, b)) * 3));
   if(bits_lost < 0)
      bits_lost = 3;
   else
      bits_lost += 3;
   int precision = bits - bits_lost;

   double min = 0;
   double max = 1;
   return boost::math::tools::halley_iterate(ibeta_roots_3<double, boost::math::policies::policy<> >(a, b, z, invert), guess, min, max, precision);
}

double inverse_ibeta_schroeder(double a, double b, double z)
{
   double guess = 0.5;
   bool invert = false;
   int bits = std::numeric_limits<double>::digits;

   //
   // special cases, we need to have these because there may be other
   // possible answers:
   //
   if(z == 1) return 1;
   if(z == 0) return 0;

   //
   // We need a good estimate of the error in the incomplete beta function
   // so that we don't set the desired precision too high.  Assume that 3-bits
   // are lost each time the arguments increase by a factor of 10:
   //
   using namespace std;
   int bits_lost = static_cast<int>(ceil(log10((std::max)(a, b)) * 3));
   if(bits_lost < 0)
      bits_lost = 3;
   else
      bits_lost += 3;
   int precision = bits - bits_lost;

   double min = 0;
   double max = 1;
   return boost::math::tools::schroeder_iterate(ibeta_roots_3<double, boost::math::policies::policy<> >(a, b, z, invert), guess, min, max, precision);
}


template <class Real, class T>
void test_inverses(const T& data)
{
   using namespace std;
   typedef typename T::value_type row_type;
   typedef Real                   value_type;

   value_type precision = static_cast<value_type>(ldexp(1.0, 1-boost::math::policies::digits<value_type, boost::math::policies::policy<> >()/2)) * 100;
   if(boost::math::policies::digits<value_type, boost::math::policies::policy<> >() < 50)
      precision = 1;   // 1% or two decimal digits, all we can hope for when the input is truncated

   for(unsigned i = 0; i < data.size(); ++i)
   {
      //
      // These inverse tests are thrown off if the output of the
      // incomplete beta is too close to 1: basically there is insuffient
      // information left in the value we're using as input to the inverse
      // to be able to get back to the original value.
      //
      if(data[i][5] == 0)
      {
         BOOST_CHECK_EQUAL(inverse_ibeta_halley(Real(data[i][0]), Real(data[i][1]), Real(data[i][5])), value_type(0));
         BOOST_CHECK_EQUAL(inverse_ibeta_schroeder(Real(data[i][0]), Real(data[i][1]), Real(data[i][5])), value_type(0));
         BOOST_CHECK_EQUAL(inverse_ibeta_newton(Real(data[i][0]), Real(data[i][1]), Real(data[i][5])), value_type(0));
         BOOST_CHECK_EQUAL(inverse_ibeta_bisect(Real(data[i][0]), Real(data[i][1]), Real(data[i][5])), value_type(0));
      }
      else if((1 - data[i][5] > 0.001) 
         && (fabs(data[i][5]) > 2 * boost::math::tools::min_value<value_type>()) 
         && (fabs(data[i][5]) > 2 * boost::math::tools::min_value<double>()))
      {
         value_type inv = inverse_ibeta_halley(Real(data[i][0]), Real(data[i][1]), Real(data[i][5]));
         BOOST_CHECK_CLOSE_EX(Real(data[i][2]), inv, precision, i);
         inv = inverse_ibeta_schroeder(Real(data[i][0]), Real(data[i][1]), Real(data[i][5]));
         BOOST_CHECK_CLOSE_EX(Real(data[i][2]), inv, precision, i);
         inv = inverse_ibeta_newton(Real(data[i][0]), Real(data[i][1]), Real(data[i][5]));
         BOOST_CHECK_CLOSE_EX(Real(data[i][2]), inv, precision, i);
         inv = inverse_ibeta_bisect(Real(data[i][0]), Real(data[i][1]), Real(data[i][5]));
         BOOST_CHECK_CLOSE_EX(Real(data[i][2]), inv, precision, i);
      }
      else if(1 == data[i][5])
      {
         BOOST_CHECK_EQUAL(inverse_ibeta_halley(Real(data[i][0]), Real(data[i][1]), Real(data[i][5])), value_type(1));
         BOOST_CHECK_EQUAL(inverse_ibeta_schroeder(Real(data[i][0]), Real(data[i][1]), Real(data[i][5])), value_type(1));
         BOOST_CHECK_EQUAL(inverse_ibeta_newton(Real(data[i][0]), Real(data[i][1]), Real(data[i][5])), value_type(1));
         BOOST_CHECK_EQUAL(inverse_ibeta_bisect(Real(data[i][0]), Real(data[i][1]), Real(data[i][5])), value_type(1));
      }

   }
}

#ifndef SC_
#define SC_(x) static_cast<typename table_type<T>::type>(BOOST_JOIN(x, L))
#endif

template <class T>
void test_beta(T, const char* /* name */)
{
   //
   // The actual test data is rather verbose, so it's in a separate file
   //
   // The contents are as follows, each row of data contains
   // five items, input value a, input value b, integration limits x, beta(a, b, x) and ibeta(a, b, x):
   //
#  include "ibeta_small_data.ipp"

   test_inverses<T>(ibeta_small_data);

#  include "ibeta_data.ipp"

   test_inverses<T>(ibeta_data);

#  include "ibeta_large_data.ipp"

   test_inverses<T>(ibeta_large_data);
}

BOOST_AUTO_TEST_CASE( test_main )
{
   test_beta(0.1, "double");
   
}



