// test_nc_beta.cpp

// Copyright John Maddock 2008.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pch.hpp>

#ifdef _MSC_VER
#pragma warning (disable:4127 4512)
#endif

#if !defined(TEST_FLOAT) && !defined(TEST_DOUBLE) && !defined(TEST_LDOUBLE) && !defined(TEST_REAL_CONCEPT)
#  define TEST_FLOAT
#  define TEST_DOUBLE
#  define TEST_LDOUBLE
#  define TEST_REAL_CONCEPT
#endif

#include <boost/math/concepts/real_concept.hpp> // for real_concept
#include <boost/math/distributions/non_central_f.hpp> // for chi_squared_distribution
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp> // for test_main
#include <boost/test/results_collector.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp> // for BOOST_CHECK_CLOSE
#include "test_out_of_range.hpp"

#include "functor.hpp"
#include "handle_test_result.hpp"

#include <iostream>
using std::cout;
using std::endl;
#include <limits>
using std::numeric_limits;

#define BOOST_CHECK_CLOSE_EX(a, b, prec, i) \
   {\
      unsigned int failures = boost::unit_test::results_collector.results( boost::unit_test::framework::current_test_case().p_id ).p_assertions_failed;\
      BOOST_CHECK_CLOSE(a, b, prec); \
      if(failures != boost::unit_test::results_collector.results( boost::unit_test::framework::current_test_case().p_id ).p_assertions_failed)\
      {\
         std::cerr << "Failure was at row " << i << std::endl;\
         std::cerr << std::setprecision(35); \
         std::cerr << "{ " << data[i][0] << " , " << data[i][1] << " , " << data[i][2];\
         std::cerr << " , " << data[i][3] << " , " << data[i][4] << " } " << std::endl;\
      }\
   }

#define BOOST_CHECK_EX(a, i) \
   {\
      unsigned int failures = boost::unit_test::results_collector.results( boost::unit_test::framework::current_test_case().p_id ).p_assertions_failed;\
      BOOST_CHECK(a); \
      if(failures != boost::unit_test::results_collector.results( boost::unit_test::framework::current_test_case().p_id ).p_assertions_failed)\
      {\
         std::cerr << "Failure was at row " << i << std::endl;\
         std::cerr << std::setprecision(35); \
         std::cerr << "{ " << data[i][0] << " , " << data[i][1] << " , " << data[i][2];\
         std::cerr << " , " << data[i][3] << " , " << data[i][4] << " } " << std::endl;\
      }\
   }

void expected_results()
{
   //
   // Define the max and mean errors expected for
   // various compilers and platforms.
   //
   const char* largest_type;
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
   if(boost::math::policies::digits<double, boost::math::policies::policy<> >() == boost::math::policies::digits<long double, boost::math::policies::policy<> >())
   {
      largest_type = "(long\\s+)?double|real_concept";
   }
   else
   {
      largest_type = "long double|real_concept";
   }
#else
   largest_type = "(long\\s+)?double|real_concept";
#endif

   //
   // Finish off by printing out the compiler/stdlib/platform names,
   // we do this to make it easier to mark up expected error rates.
   //
   std::cout << "Tests run with " << BOOST_COMPILER << ", " 
      << BOOST_STDLIB << ", " << BOOST_PLATFORM << std::endl;
}

template <class RealType>
RealType naive_pdf(RealType v1, RealType v2, RealType l, RealType f)
{
   BOOST_MATH_STD_USING
   RealType sum = 0;
   for(int i = 0; ; ++i)
   {
      RealType term = -l/2 + log(l/2) * i;
      term += boost::math::lgamma(v2/2 + v1/2+i) - (boost::math::lgamma(v2/2) + boost::math::lgamma(v1/2+i));
      term -= boost::math::lgamma(RealType(i+1));
      term += log(v1/v2) * (v1/2+i) + log(v2 / (v2 + v1 * f)) * ((v1 + v2) / 2 + i);
      term += log(f) * (v1/2 - 1 + i);
      term = exp(term);
      sum += term;
      if((term/sum < boost::math::tools::epsilon<RealType>()) || (term == 0))
         break;
   }
   return sum;
}

template <class RealType>
void test_spot(
     RealType a,     // df1
     RealType b,     // df2
     RealType ncp,   // non-centrality param
     RealType x,     // F statistic
     RealType P,     // CDF
     RealType Q,     // Complement of CDF
     RealType D,     // PDF
     RealType tol)   // Test tolerance
{
   boost::math::non_central_f_distribution<RealType> dist(a, b, ncp);
   BOOST_CHECK_CLOSE(
      cdf(dist, x), P, tol);
   BOOST_CHECK_CLOSE(
      pdf(dist, x), D, tol);
   if(boost::math::tools::digits<RealType>() > 50)
   {
      //
      // The "naive" pdf calculation fails at float precision.
      //
      BOOST_CHECK_CLOSE(
         pdf(dist, x), naive_pdf(a, b, ncp, x), tol);
   }

   if((P < 0.99) && (Q < 0.99))
   {
      //
      // We can only check this if P is not too close to 1,
      // so that we can guarentee Q is reasonably free of error:
      //
      BOOST_CHECK_CLOSE(
         cdf(complement(dist, x)), Q, tol);
      BOOST_CHECK_CLOSE(
            quantile(dist, P), x, tol * 10);
      BOOST_CHECK_CLOSE(
            quantile(complement(dist, Q)), x, tol * 10);
   }
   if(boost::math::tools::digits<RealType>() > 50)
   {
      //
      // Sanity check mode:
      //
      RealType m = mode(dist);
      RealType p = pdf(dist, m);
      BOOST_CHECK(pdf(dist, m * (1 + sqrt(tol) * 10)) <= p);
      BOOST_CHECK(pdf(dist, m * (1 - sqrt(tol)) * 10) <= p);
   }
}

template <class RealType> // Any floating-point type RealType.
void test_spots(RealType)
{
   RealType tolerance = (std::max)(
      boost::math::tools::epsilon<RealType>() * 100,
      (RealType)1e-6) * 100;

   cout << "Tolerance = " << tolerance << "%." << endl;

   //
   // Spot tests use values computed by the R statistical
   // package and the pbeta and dbeta functions:
   //
   test_spot(
     RealType(2),                   // alpha
     RealType(5),                   // beta
     RealType(1),                   // non-centrality param
     RealType(2),                   // F statistic
     RealType(0.6493871),           // CDF
     RealType(1-0.6493871),         // Complement of CDF
     RealType(0.1551262),           // PDF
     RealType(tolerance));
   test_spot(
     RealType(100),                 // alpha
     RealType(5),                   // beta
     RealType(15),                  // non-centrality param
     RealType(105),                 // F statistic
     RealType(0.999962),            // CDF
     RealType(1-0.999962),          // Complement of CDF
     RealType(8.95623e-07),         // PDF
     RealType(tolerance));
   test_spot(
     RealType(100),                 // alpha
     RealType(5),                   // beta
     RealType(15),                  // non-centrality param
     RealType(1.5),                 // F statistic
     RealType(0.5759232),           // CDF
     RealType(1-0.5759232),         // Complement of CDF
     RealType(0.3674375),           // PDF
     RealType(tolerance));
   test_spot(
     RealType(5),                   // alpha
     RealType(100),                 // beta
     RealType(102),                 // non-centrality param
     RealType(25),                  // F statistic
     RealType(0.7499338),           // CDF
     RealType(1-0.7499338),         // Complement of CDF
     RealType(0.0544676),           // PDF
     RealType(tolerance));
   test_spot(
     RealType(85),                  // alpha
     RealType(100),                 // beta
     RealType(245),                 // non-centrality param
     RealType(3.5),                 // F statistic
     RealType(0.2697244),           // CDF
     RealType(1-0.2697244),         // Complement of CDF
     RealType(0.5435237),           // PDF
     RealType(tolerance));
   test_spot(
     RealType(85),                  // alpha
     RealType(100),                 // beta
     RealType(0.5),                 // non-centrality param
     RealType(1.25),                // F statistic
     RealType(0.8522862),           // CDF
     RealType(1-0.8522862),         // Complement of CDF
     RealType(0.8851028),           // PDF
     RealType(tolerance));

   BOOST_MATH_STD_USING

   //
   // 5 eps expressed as a persentage, otherwise the limit of the test data:
   //
   RealType tol2 = (std::max)(boost::math::tools::epsilon<RealType>() * 500, RealType(1e-25));
   RealType x = 2;
   
   boost::math::non_central_f_distribution<RealType> dist(20, 15, 30);
   // mean:
   BOOST_CHECK_CLOSE(
      mean(dist)
      , static_cast<RealType>(2.8846153846153846153846153846154L), tol2);
   // variance:
   BOOST_CHECK_CLOSE(
      variance(dist)
      , static_cast<RealType>(2.1422807961269499731038192576654L), tol2);
   // std deviation:
   BOOST_CHECK_CLOSE(
      standard_deviation(dist)
      , sqrt(variance(dist)), tol2);
   // hazard:
   BOOST_CHECK_CLOSE(
      hazard(dist, x)
      , pdf(dist, x) / cdf(complement(dist, x)), tol2);
   // cumulative hazard:
   BOOST_CHECK_CLOSE(
      chf(dist, x)
      , -log(cdf(complement(dist, x))), tol2);
   // coefficient_of_variation:
   BOOST_CHECK_CLOSE(
      coefficient_of_variation(dist)
      , standard_deviation(dist) / mean(dist), tol2);
   BOOST_CHECK_CLOSE(
      median(dist), 
      quantile(
      dist,
      static_cast<RealType>(0.5)), static_cast<RealType>(tol2));
   // mode:
   BOOST_CHECK_CLOSE(
      mode(dist)
      , static_cast<RealType>(2.070019130232759428074835788815387743293972985542L), sqrt(tolerance));
   // skewness:
   BOOST_CHECK_CLOSE(
      skewness(dist)
      , static_cast<RealType>(2.1011821125804540669752380228510691320707051692719L), tol2);
   // kurtosis:
   BOOST_CHECK_CLOSE(
      kurtosis(dist)
      , 3 + kurtosis_excess(dist), tol2);
   // kurtosis excess:
   BOOST_CHECK_CLOSE(
      kurtosis_excess(dist)
      , static_cast<RealType>(13.225781681053154767604638331440974359675882226873L), tol2);

   // Error handling checks:
   check_out_of_range<boost::math::non_central_f_distribution<RealType> >(1, 1, 1);
   BOOST_CHECK_THROW(pdf(boost::math::non_central_f_distribution<RealType>(0, 1, 1), 0), std::domain_error);
   BOOST_CHECK_THROW(pdf(boost::math::non_central_f_distribution<RealType>(-1, 1, 1), 0), std::domain_error);
   BOOST_CHECK_THROW(pdf(boost::math::non_central_f_distribution<RealType>(1, 0, 1), 0), std::domain_error);
   BOOST_CHECK_THROW(pdf(boost::math::non_central_f_distribution<RealType>(1, -1, 1), 0), std::domain_error);
   BOOST_CHECK_THROW(quantile(boost::math::non_central_f_distribution<RealType>(1, 1, 1), -1), std::domain_error);
   BOOST_CHECK_THROW(quantile(boost::math::non_central_f_distribution<RealType>(1, 1, 1), 2), std::domain_error);
} // template <class RealType>void test_spots(RealType)

BOOST_AUTO_TEST_CASE( test_main )
{
   BOOST_MATH_CONTROL_FP;
   // Basic sanity-check spot values.
   expected_results();
   // (Parameter value, arbitrarily zero, only communicates the floating point type).
   test_spots(0.0F); // Test float.
   test_spots(0.0); // Test double.
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
   test_spots(0.0L); // Test long double.
#if !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x582))
   test_spots(boost::math::concepts::real_concept(0.)); // Test real concept.
#endif
#endif

   
} // BOOST_AUTO_TEST_CASE( test_main )

