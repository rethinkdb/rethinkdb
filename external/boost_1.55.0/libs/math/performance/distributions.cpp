//  Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "required_defines.hpp"

#include "performance_measure.hpp"

#include <boost/math/distributions.hpp>

double probabilities[] = {
   1e-5,
   1e-4,
   1e-3,
   1e-2,
   0.05,
   0.1,
   0.2,
   0.3,
   0.4,
   0.5,
   0.6,
   0.7,
   0.8,
   0.9,
   0.95,
   1-1e-5,
   1-1e-4,
   1-1e-3,
   1-1e-2
};

int int_values[] = {
   1,
   2,
   3,
   5,
   10,
   20,
   50,
   100,
   1000,
   10000,
   100000
};

int small_int_values[] = {
   1,
   2,
   3,
   5,
   10,
   15,
   20,
   30,
   50,
   100,
   150
};

double real_values[] = {
   1e-5,
   1e-4,
   1e-2,
   1e-1,
   1,
   10,
   100,
   1000,
   10000,
   100000
};

#define BOOST_MATH_DISTRIBUTION3_TEST(name, param1_table, param2_table, param3_table, random_variable_table, probability_table) \
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist, name), "dist-" BOOST_STRINGIZE(name) "-cdf")\
   {\
   double result = 0;\
   unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
   unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
   unsigned c_size = sizeof(param3_table)/sizeof(param3_table[0]);\
   unsigned d_size = sizeof(random_variable_table)/sizeof(random_variable_table[0]);\
   \
   for(unsigned i = 0; i < a_size; ++i)\
   {\
      for(unsigned j = 0; j < b_size; ++j)\
      {\
         for(unsigned k = 0; k < c_size; ++k)\
         {\
            for(unsigned l = 0; l < d_size; ++l)\
            {\
               result += cdf(boost::math:: BOOST_JOIN(name, _distribution) <>(param1_table[i], param2_table[j], param3_table[k]), random_variable_table[l]);\
            }\
         }\
      }\
   }\
   \
   consume_result(result);\
   set_call_count(a_size * b_size * c_size * d_size);\
   }\
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist_pdf_, name), "dist-" BOOST_STRINGIZE(name) "-pdf")\
   {\
   double result = 0;\
   unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
   unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
   unsigned c_size = sizeof(param3_table)/sizeof(param3_table[0]);\
   unsigned d_size = sizeof(random_variable_table)/sizeof(random_variable_table[0]);\
   \
   for(unsigned i = 0; i < a_size; ++i)\
   {\
      for(unsigned j = 0; j < b_size; ++j)\
      {\
         for(unsigned k = 0; k < c_size; ++k)\
         {\
            for(unsigned l = 0; l < d_size; ++l)\
            {\
               result += pdf(boost::math:: BOOST_JOIN(name, _distribution) <>(param1_table[i], param2_table[j], param3_table[k]), random_variable_table[l]);\
            }\
         }\
      }\
   }\
   \
   consume_result(result);\
   set_call_count(a_size * b_size * c_size * d_size);\
   }\
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist_quant, name), "dist-" BOOST_STRINGIZE(name) "-quantile")\
   {\
      double result = 0;\
      unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
      unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
      unsigned c_size = sizeof(param3_table)/sizeof(param3_table[0]);\
      unsigned d_size = sizeof(probability_table)/sizeof(probability_table[0]);\
      \
      for(unsigned i = 0; i < a_size; ++i)\
      {\
         for(unsigned j = 0; j < b_size; ++j)\
         {\
            for(unsigned k = 0; k < c_size; ++k)\
            {\
               for(unsigned l = 0; l < d_size; ++l)\
               {\
                  result += quantile(boost::math:: BOOST_JOIN(name, _distribution) <>(param1_table[i], param2_table[j], param3_table[k]), probability_table[l]);\
               }\
            }\
         }\
      }\
      \
      consume_result(result);\
      set_call_count(a_size * b_size * c_size * d_size);\
   }

#define BOOST_MATH_DISTRIBUTION2_TEST(name, param1_table, param2_table, random_variable_table, probability_table) \
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist, name), "dist-" BOOST_STRINGIZE(name) "-cdf")\
   {\
   double result = 0;\
   unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
   unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
   unsigned c_size = sizeof(random_variable_table)/sizeof(random_variable_table[0]);\
   \
   for(unsigned i = 0; i < a_size; ++i)\
   {\
      for(unsigned j = 0; j < b_size; ++j)\
      {\
         for(unsigned k = 0; k < c_size; ++k)\
         {\
            result += cdf(boost::math:: BOOST_JOIN(name, _distribution) <>(param1_table[i], param2_table[j]), random_variable_table[k]);\
         }\
      }\
   }\
   \
   consume_result(result);\
   set_call_count(a_size * b_size * c_size);\
   }\
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist_pdf_, name), "dist-" BOOST_STRINGIZE(name) "-pdf")\
   {\
   double result = 0;\
   unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
   unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
   unsigned c_size = sizeof(random_variable_table)/sizeof(random_variable_table[0]);\
   \
   for(unsigned i = 0; i < a_size; ++i)\
   {\
      for(unsigned j = 0; j < b_size; ++j)\
      {\
         for(unsigned k = 0; k < c_size; ++k)\
         {\
            result += pdf(boost::math:: BOOST_JOIN(name, _distribution) <>(param1_table[i], param2_table[j]), random_variable_table[k]);\
         }\
      }\
   }\
   \
   consume_result(result);\
   set_call_count(a_size * b_size * c_size);\
   }\
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist_quant, name), "dist-" BOOST_STRINGIZE(name) "-quantile")\
   {\
      double result = 0;\
      unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
      unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
      unsigned c_size = sizeof(probability_table)/sizeof(probability_table[0]);\
      \
      for(unsigned i = 0; i < a_size; ++i)\
      {\
         for(unsigned j = 0; j < b_size; ++j)\
         {\
            for(unsigned k = 0; k < c_size; ++k)\
            {\
               result += quantile(boost::math:: BOOST_JOIN(name, _distribution) <>(param1_table[i], param2_table[j]), probability_table[k]);\
            }\
         }\
      }\
      \
      consume_result(result);\
      set_call_count(a_size * b_size * c_size);\
   }

#define BOOST_MATH_DISTRIBUTION1_TEST(name, param1_table, random_variable_table, probability_table) \
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist, name), "dist-" BOOST_STRINGIZE(name) "-cdf")\
   {\
   double result = 0;\
   unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
   unsigned c_size = sizeof(random_variable_table)/sizeof(random_variable_table[0]);\
   \
   for(unsigned i = 0; i < a_size; ++i)\
   {\
         for(unsigned k = 0; k < c_size; ++k)\
         {\
            result += cdf(boost::math:: BOOST_JOIN(name, _distribution) <>(param1_table[i]), random_variable_table[k]);\
         }\
   }\
   \
   consume_result(result);\
   set_call_count(a_size * c_size);\
   }\
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist_pdf_, name), "dist-" BOOST_STRINGIZE(name) "-pdf")\
   {\
   double result = 0;\
   unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
   unsigned c_size = sizeof(random_variable_table)/sizeof(random_variable_table[0]);\
   \
   for(unsigned i = 0; i < a_size; ++i)\
   {\
         for(unsigned k = 0; k < c_size; ++k)\
         {\
            result += pdf(boost::math:: BOOST_JOIN(name, _distribution) <>(param1_table[i]), random_variable_table[k]);\
         }\
   }\
   \
   consume_result(result);\
   set_call_count(a_size * c_size);\
   }\
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist_quant, name), "dist-" BOOST_STRINGIZE(name) "-quantile")\
   {\
      double result = 0;\
      unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
      unsigned c_size = sizeof(probability_table)/sizeof(probability_table[0]);\
      \
      for(unsigned i = 0; i < a_size; ++i)\
      {\
            for(unsigned k = 0; k < c_size; ++k)\
            {\
               result += quantile(boost::math:: BOOST_JOIN(name, _distribution) <>(param1_table[i]), probability_table[k]);\
            }\
      }\
      \
      consume_result(result);\
      set_call_count(a_size * c_size);\
   }

BOOST_MATH_DISTRIBUTION2_TEST(beta, probabilities, probabilities, probabilities, probabilities)
BOOST_MATH_DISTRIBUTION2_TEST(binomial, int_values, probabilities, int_values, probabilities)
BOOST_MATH_DISTRIBUTION2_TEST(cauchy, int_values, real_values, int_values, probabilities)
BOOST_MATH_DISTRIBUTION1_TEST(chi_squared, int_values, real_values, probabilities)
BOOST_MATH_DISTRIBUTION1_TEST(exponential, real_values, real_values, probabilities)
BOOST_MATH_DISTRIBUTION2_TEST(fisher_f, int_values, int_values, real_values, probabilities)
BOOST_MATH_DISTRIBUTION2_TEST(gamma, real_values, real_values, real_values, probabilities)
BOOST_MATH_DISTRIBUTION3_TEST(hypergeometric, small_int_values, small_int_values, small_int_values, small_int_values, probabilities)
BOOST_MATH_DISTRIBUTION2_TEST(logistic, real_values, real_values, real_values, probabilities)
BOOST_MATH_DISTRIBUTION2_TEST(lognormal, real_values, real_values, real_values, probabilities)
BOOST_MATH_DISTRIBUTION2_TEST(negative_binomial, int_values, probabilities, int_values, probabilities)
BOOST_MATH_DISTRIBUTION2_TEST(normal, real_values, real_values, real_values, probabilities)
BOOST_MATH_DISTRIBUTION1_TEST(poisson, real_values, int_values, probabilities)
BOOST_MATH_DISTRIBUTION1_TEST(students_t, int_values, real_values, probabilities)
BOOST_MATH_DISTRIBUTION2_TEST(weibull, real_values, real_values, real_values, probabilities)
BOOST_MATH_DISTRIBUTION2_TEST(non_central_chi_squared, int_values, int_values, real_values, probabilities)
BOOST_MATH_DISTRIBUTION3_TEST(non_central_beta, int_values, int_values, real_values, real_values, probabilities)
BOOST_MATH_DISTRIBUTION3_TEST(non_central_f, int_values, int_values, real_values, real_values, probabilities)
BOOST_MATH_DISTRIBUTION2_TEST(non_central_t, int_values, small_int_values, real_values, probabilities)

#ifdef TEST_R

#define MATHLIB_STANDALONE 1

extern "C" {
#include "Rmath.h"
}

#define BOOST_MATH_R_DISTRIBUTION3_TEST(name, param1_table, param2_table, param3_table, random_variable_table, probability_table) \
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist, name), "dist-" #name "-R-cdf")\
   {\
   double result = 0;\
   unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
   unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
   unsigned c_size = sizeof(param3_table)/sizeof(param3_table[0]);\
   unsigned d_size = sizeof(random_variable_table)/sizeof(random_variable_table[0]);\
   \
   for(unsigned i = 0; i < a_size; ++i)\
   {\
      for(unsigned j = 0; j < b_size; ++j)\
      {\
         for(unsigned k = 0; k < c_size; ++k)\
         {\
            for(unsigned l = 0; l < d_size; ++l)\
            {\
               result += p##name (random_variable_table[l], param1_table[i], param2_table[j], param3_table[k], 1, 0);\
            }\
         }\
      }\
   }\
   \
   consume_result(result);\
   set_call_count(a_size * b_size * c_size * d_size);\
   }\
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist_pdf_, name), "dist-" #name "-R-pdf")\
   {\
   double result = 0;\
   unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
   unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
   unsigned c_size = sizeof(param3_table)/sizeof(param3_table[0]);\
   unsigned d_size = sizeof(random_variable_table)/sizeof(random_variable_table[0]);\
   \
   for(unsigned i = 0; i < a_size; ++i)\
   {\
      for(unsigned j = 0; j < b_size; ++j)\
      {\
         for(unsigned k = 0; k < c_size; ++k)\
         {\
            for(unsigned l = 0; l < d_size; ++l)\
            {\
               result += d##name (random_variable_table[l], param1_table[i], param2_table[j], param3_table[k], 0);\
            }\
         }\
      }\
   }\
   \
   consume_result(result);\
   set_call_count(a_size * b_size * c_size * d_size);\
   }\
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist_quant, name), "dist-" #name "-R-quantile")\
   {\
      double result = 0;\
      unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
      unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
      unsigned c_size = sizeof(param3_table)/sizeof(param3_table[0]);\
      unsigned d_size = sizeof(probability_table)/sizeof(probability_table[0]);\
      \
      for(unsigned i = 0; i < a_size; ++i)\
      {\
         for(unsigned j = 0; j < b_size; ++j)\
         {\
            for(unsigned k = 0; k < c_size; ++k)\
            {\
               for(unsigned l = 0; l < d_size; ++l)\
               {\
                  result += q##name (probability_table[l], param1_table[i], param2_table[j], param3_table[k], 1, 0);\
               }\
            }\
         }\
      }\
      \
      consume_result(result);\
      set_call_count(a_size * b_size * c_size * d_size);\
   }

#define BOOST_MATH_R_DISTRIBUTION2_TEST(name, param1_table, param2_table, random_variable_table, probability_table) \
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist, name), "dist-" #name "-R-cdf")\
   {\
   double result = 0;\
   unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
   unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
   unsigned c_size = sizeof(random_variable_table)/sizeof(random_variable_table[0]);\
   \
   for(unsigned i = 0; i < a_size; ++i)\
   {\
      for(unsigned j = 0; j < b_size; ++j)\
      {\
         for(unsigned k = 0; k < c_size; ++k)\
         {\
            result += p##name (random_variable_table[k], param1_table[i], param2_table[j], 1, 0);\
         }\
      }\
   }\
   \
   consume_result(result);\
   set_call_count(a_size * b_size * c_size);\
   }\
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist_pdf_, name), "dist-" #name "-R-pdf")\
   {\
   double result = 0;\
   unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
   unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
   unsigned c_size = sizeof(random_variable_table)/sizeof(random_variable_table[0]);\
   \
   for(unsigned i = 0; i < a_size; ++i)\
   {\
      for(unsigned j = 0; j < b_size; ++j)\
      {\
         for(unsigned k = 0; k < c_size; ++k)\
         {\
            result += d##name (random_variable_table[k], param1_table[i], param2_table[j], 0);\
         }\
      }\
   }\
   \
   consume_result(result);\
   set_call_count(a_size * b_size * c_size);\
   }\
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist_quant, name), "dist-" #name "-R-quantile")\
   {\
      double result = 0;\
      unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
      unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
      unsigned c_size = sizeof(probability_table)/sizeof(probability_table[0]);\
      \
      for(unsigned i = 0; i < a_size; ++i)\
      {\
         for(unsigned j = 0; j < b_size; ++j)\
         {\
            for(unsigned k = 0; k < c_size; ++k)\
            {\
               result += q##name (probability_table[k], param1_table[i], param2_table[j], 1, 0);\
            }\
         }\
      }\
      \
      consume_result(result);\
      set_call_count(a_size * b_size * c_size);\
   }

#define BOOST_MATH_R_DISTRIBUTION1_TEST(name, param1_table, random_variable_table, probability_table) \
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist, name), "dist-" #name "-R-cdf")\
   {\
   double result = 0;\
   unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
   unsigned c_size = sizeof(random_variable_table)/sizeof(random_variable_table[0]);\
   \
   for(unsigned i = 0; i < a_size; ++i)\
   {\
         for(unsigned k = 0; k < c_size; ++k)\
         {\
            result += p##name (random_variable_table[k], param1_table[i], 1, 0);\
         }\
   }\
   \
   consume_result(result);\
   set_call_count(a_size * c_size);\
   }\
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist_pdf_, name), "dist-" #name "-R-pdf")\
   {\
   double result = 0;\
   unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
   unsigned c_size = sizeof(random_variable_table)/sizeof(random_variable_table[0]);\
   \
   for(unsigned i = 0; i < a_size; ++i)\
   {\
         for(unsigned k = 0; k < c_size; ++k)\
         {\
            result += d##name (random_variable_table[k], param1_table[i], 0);\
         }\
   }\
   \
   consume_result(result);\
   set_call_count(a_size * c_size);\
   }\
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist_quant, name), "dist-" #name "-R-quantile")\
   {\
      double result = 0;\
      unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
      unsigned c_size = sizeof(probability_table)/sizeof(probability_table[0]);\
      \
      for(unsigned i = 0; i < a_size; ++i)\
      {\
            for(unsigned k = 0; k < c_size; ++k)\
            {\
               result += q##name (probability_table[k], param1_table[i], 1, 0);\
            }\
      }\
      \
      consume_result(result);\
      set_call_count(a_size * c_size);\
   }

double qhypergeo(double r, double n, double N, double p, int i, int j)
{
   if(r > N)
      return std::numeric_limits<double>::quiet_NaN();
   double nr = r;
   double nb = N - r;
   return qhyper(nr, nb, n, p, i, j);
}

double phypergeo(double r, double n, double N, double k, int i, int j)
{
   if((r > N) || (k > n) || (k > r))
      return std::numeric_limits<double>::quiet_NaN();
   double nr = r;
   double nb = N - r;
   return phyper(nr, nb, n, k, i, j);
}

double dhypergeo(double r, double n, double N, double k, int i)
{
   if((r > N) || (k > n) || (k > r))
      return std::numeric_limits<double>::quiet_NaN();
   double nr = r;
   double nb = N - r;
   return dhyper(nr, nb, n, k, i);
}

BOOST_MATH_R_DISTRIBUTION2_TEST(beta, probabilities, probabilities, probabilities, probabilities)
BOOST_MATH_R_DISTRIBUTION2_TEST(binom, int_values, probabilities, int_values, probabilities)
BOOST_MATH_R_DISTRIBUTION2_TEST(cauchy, int_values, real_values, int_values, probabilities)
BOOST_MATH_R_DISTRIBUTION1_TEST(chisq, int_values, real_values, probabilities)
BOOST_MATH_R_DISTRIBUTION1_TEST(exp, real_values, real_values, probabilities)
BOOST_MATH_R_DISTRIBUTION2_TEST(f, int_values, int_values, real_values, probabilities)
BOOST_MATH_R_DISTRIBUTION2_TEST(gamma, real_values, real_values, real_values, probabilities)
BOOST_MATH_R_DISTRIBUTION3_TEST(hypergeo, small_int_values, small_int_values, small_int_values, small_int_values, probabilities)
BOOST_MATH_R_DISTRIBUTION2_TEST(logis, real_values, real_values, real_values, probabilities)
BOOST_MATH_R_DISTRIBUTION2_TEST(lnorm, real_values, real_values, real_values, probabilities)
BOOST_MATH_R_DISTRIBUTION2_TEST(nbinom, int_values, probabilities, int_values, probabilities)
BOOST_MATH_R_DISTRIBUTION2_TEST(norm, real_values, real_values, real_values, probabilities)
BOOST_MATH_R_DISTRIBUTION1_TEST(pois, real_values, int_values, probabilities)
BOOST_MATH_R_DISTRIBUTION1_TEST(t, int_values, real_values, probabilities)
BOOST_MATH_R_DISTRIBUTION2_TEST(weibull, real_values, real_values, real_values, probabilities)
BOOST_MATH_R_DISTRIBUTION2_TEST(nchisq, int_values, int_values, real_values, probabilities)
BOOST_MATH_R_DISTRIBUTION3_TEST(nf, int_values, int_values, real_values, real_values, probabilities)
BOOST_MATH_R_DISTRIBUTION3_TEST(nbeta, int_values, int_values, real_values, real_values, probabilities)
BOOST_MATH_R_DISTRIBUTION2_TEST(nt, int_values, small_int_values, real_values, probabilities)

#endif

#ifdef TEST_CEPHES

extern "C"{

double bdtr(int k, int n, double p);
double bdtri(int k, int n, double p);

double chdtr(double df, double x);
double chdtri(double df, double p);

double fdtr(int k, int n, double p);
double fdtri(int k, int n, double p);

double nbdtr(int k, int n, double p);
double nbdtri(int k, int n, double p);

}

#define BOOST_MATH_CEPHES_DISTRIBUTION2_TEST(name, param1_table, param2_table, random_variable_table, probability_table) \
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist, name), "dist-" #name "-cephes-cdf")\
   {\
   double result = 0;\
   unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
   unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
   unsigned c_size = sizeof(random_variable_table)/sizeof(random_variable_table[0]);\
   \
   for(unsigned i = 0; i < a_size; ++i)\
   {\
      for(unsigned j = 0; j < b_size; ++j)\
      {\
         for(unsigned k = 0; k < c_size; ++k)\
         {\
            result += name##dtr (param1_table[i], param2_table[j], random_variable_table[k]);\
         }\
      }\
   }\
   \
   consume_result(result);\
   set_call_count(a_size * b_size * c_size);\
   }\
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist_quant, name), "dist-" #name "-cephes-quantile")\
   {\
      double result = 0;\
      unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
      unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
      unsigned c_size = sizeof(probability_table)/sizeof(probability_table[0]);\
      \
      for(unsigned i = 0; i < a_size; ++i)\
      {\
         for(unsigned j = 0; j < b_size; ++j)\
         {\
            for(unsigned k = 0; k < c_size; ++k)\
            {\
               result += name##dtri (param1_table[i], param2_table[j], probability_table[k]);\
            }\
         }\
      }\
      \
      consume_result(result);\
      set_call_count(a_size * b_size * c_size);\
   }

#define BOOST_MATH_CEPHES_DISTRIBUTION1_TEST(name, param1_table, random_variable_table, probability_table) \
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist, name), "dist-" #name "-cephes-cdf")\
   {\
   double result = 0;\
   unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
   unsigned c_size = sizeof(random_variable_table)/sizeof(random_variable_table[0]);\
   \
   for(unsigned i = 0; i < a_size; ++i)\
   {\
         for(unsigned k = 0; k < c_size; ++k)\
         {\
            result += name##dtr (param1_table[i], random_variable_table[k]);\
         }\
   }\
   \
   consume_result(result);\
   set_call_count(a_size * c_size);\
   }\
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dist_quant, name), "dist-" #name "-cephes-quantile")\
   {\
      double result = 0;\
      unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
      unsigned c_size = sizeof(probability_table)/sizeof(probability_table[0]);\
      \
      for(unsigned i = 0; i < a_size; ++i)\
      {\
            for(unsigned k = 0; k < c_size; ++k)\
            {\
               result += name##dtri (param1_table[i], probability_table[k]);\
            }\
      }\
      \
      consume_result(result);\
      set_call_count(a_size * c_size);\
   }
// Cephes inverse doesn't actually calculate the quantile!!!
// BOOST_MATH_CEPHES_DISTRIBUTION2_TEST(b, int_values, int_values, probabilities, probabilities)
BOOST_MATH_CEPHES_DISTRIBUTION1_TEST(ch, int_values, real_values, probabilities)
BOOST_MATH_CEPHES_DISTRIBUTION2_TEST(f, int_values, int_values, real_values, probabilities)
// Cephes inverse doesn't calculate the quantile!!!
// BOOST_MATH_CEPHES_DISTRIBUTION2_TEST(nb, int_values, int_values, probabilities, probabilities)

#endif

#ifdef TEST_DCDFLIB
#include <dcdflib.h>

void cdfbeta( int *which, double *p, double *q, double *x, double *a, double *b, int *status, double *bound)
{
   double y = 1 - *x;
   cdfbet(which, p, q, x, &y, a, b, status, bound);
}

void cdfbinomial( int *which, double *p, double *q, double *x, double *a, double *b, int *status, double *bound)
{
   double y = 1 - *x;
   double cb = 1 - *b;
   cdfbet(which, p, q, x, a, b, &cb, status, bound);
}

void cdfnegative_binomial( int *which, double *p, double *q, double *x, double *a, double *b, int *status, double *bound)
{
   double y = 1 - *x;
   double cb = 1 - *b;
   cdfnbn(which, p, q, x, a, b, &cb, status, bound);
}

void cdfchi_squared( int *which, double *p, double *q, double *x, double *a, int *status, double *bound)
{
   cdfchi(which, p, q, x, a, status, bound);
}

void cdfnon_central_chi_squared( int *which, double *p, double *q, double *x, double *a, double *b, int *status, double *bound)
{
   cdfchn(which, p, q, x, a, b, status, bound);
}

namespace boost{ namespace math{

   template <class T = double> struct f_distribution : public fisher_f_distribution<T> 
   { f_distribution(T a, T b) : fisher_f_distribution<T>(a, b) {} };
   template <class T = double> 
   struct fnc_distribution : public non_central_f_distribution<T> 
   { fnc_distribution(T a, T b, T c) : non_central_f_distribution<T>(a, b, c) {} };
   template <class T = double> struct gam_distribution : public gamma_distribution<T> 
   { gam_distribution(T a, T b) : gamma_distribution<T>(a, b) {} };
   template <class T = double> struct nor_distribution : public normal_distribution<T> 
   { nor_distribution(T a, T b) : normal_distribution<T>(a, b) {} };
   template <class T = double> struct poi_distribution : public poisson_distribution<T> 
   { poi_distribution(T a) : poisson_distribution<T>(a) {} };
   template <class T = double> struct t_distribution : public students_t_distribution<T> 
   { t_distribution(T a) : students_t_distribution<T>(a) {} };

   template <class T>
   T cdf(const f_distribution<T>& d, const T& r){  return cdf(static_cast<fisher_f_distribution<T> >(d), r);  }
   template <class T>
   T quantile(const f_distribution<T>& d, const T& r){  return quantile(static_cast<fisher_f_distribution<T> >(d), r);  }
   template <class T>
   T cdf(const fnc_distribution<T>& d, const T& r){  return cdf(static_cast<non_central_f_distribution<T> >(d), r);  }
   template <class T>
   T quantile(const fnc_distribution<T>& d, const T& r){  return quantile(static_cast<non_central_f_distribution<T> >(d), r);  }
   template <class T>
   T cdf(const gam_distribution<T>& d, const T& r){  return cdf(static_cast<gamma_distribution<T> >(d), r);  }
   template <class T>
   T quantile(const gam_distribution<T>& d, const T& r){  return quantile(static_cast<gamma_distribution<T> >(d), r);  }
   template <class T>
   T cdf(const nor_distribution<T>& d, const T& r){  return cdf(static_cast<normal_distribution<T> >(d), r);  }
   template <class T>
   T quantile(const nor_distribution<T>& d, const T& r){  return quantile(static_cast<normal_distribution<T> >(d), r);  }
   template <class T>
   T cdf(const poi_distribution<T>& d, const T& r){  return cdf(static_cast<poisson_distribution<T> >(d), r);  }
   template <class T>
   T quantile(const poi_distribution<T>& d, const T& r){  return quantile(static_cast<poisson_distribution<T> >(d), r);  }
   template <class T>
   T cdf(const t_distribution<T>& d, const T& r){  return cdf(static_cast<students_t_distribution<T> >(d), r);  }
   template <class T>
   T quantile(const t_distribution<T>& d, const T& r){  return quantile(static_cast<students_t_distribution<T> >(d), r);  }

}}

bool check_near(double a, double b)
{
   bool r = ((fabs(a) <= 1e-7) || (fabs(b) <= 1e-7)) ? (fabs(a-b) < 1e-7) : fabs((a - b) / a) < 1e-5;
   return r;
}

#define BOOST_MATH_DCD_DISTRIBUTION3_TEST(name, param1_table, param2_table, param3_table, random_variable_table, probability_table) \
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dcd_dist, name), "dist-" BOOST_STRINGIZE(name) "-dcd-cdf")\
   {\
   double result = 0;\
   unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
   unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
   unsigned c_size = sizeof(param3_table)/sizeof(param3_table[0]);\
   unsigned d_size = sizeof(random_variable_table)/sizeof(random_variable_table[0]);\
   \
   for(unsigned i = 0; i < a_size; ++i)\
   {\
      for(unsigned j = 0; j < b_size; ++j)\
      {\
         for(unsigned k = 0; k < c_size; ++k)\
         {\
            for(unsigned l = 0; l < d_size; ++l)\
            {\
               int which = 1;\
               double p; double q; \
               double rv = random_variable_table[l];\
               double a = param1_table[i];\
               double b = param2_table[j];\
               double c = param3_table[k];\
               int status = 0;\
               double bound = 0;\
               BOOST_JOIN(cdf, name)(&which, &p, &q, &rv, &a, &b, &c, &status, &bound);\
               result += p;\
               BOOST_ASSERT(\
                  (status != 0) || check_near(p, \
                             cdf(boost::math:: BOOST_JOIN(name, _distribution) <>(param1_table[i], param2_table[j], param3_table[k]), random_variable_table[l])\
               ));\
            }\
         }\
      }\
   }\
   \
   consume_result(result);\
   set_call_count(a_size * b_size * c_size * d_size);\
   }\
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dcd_dist_quant, name), "dist-" BOOST_STRINGIZE(name) "-dcd-quantile")\
   {\
      double result = 0;\
      unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
      unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
      unsigned c_size = sizeof(param3_table)/sizeof(param3_table[0]);\
      unsigned d_size = sizeof(probability_table)/sizeof(probability_table[0]);\
      \
      for(unsigned i = 0; i < a_size; ++i)\
      {\
         for(unsigned j = 0; j < b_size; ++j)\
         {\
            for(unsigned k = 0; k < c_size; ++k)\
            {\
               for(unsigned l = 0; l < d_size; ++l)\
               {\
                  int which = 2;\
                  double p = probability_table[l];\
                  double q = 1 - p; \
                  double rv;\
                  double a = param1_table[i];\
                  double b = param2_table[j];\
                  double c = param3_table[k];\
                  int status = 0;\
                  double bound = 0;\
                  BOOST_JOIN(cdf, name)(&which, &p, &q, &rv, &a, &b, &c, &status, &bound);\
                  result += rv;\
                  BOOST_ASSERT((status != 0) || (p > 0.99) || check_near(rv, quantile(boost::math:: BOOST_JOIN(name, _distribution) <>(param1_table[i], param2_table[j], param3_table[k]), probability_table[l])));\
               }\
            }\
         }\
      }\
      \
      consume_result(result);\
      set_call_count(a_size * b_size * c_size * d_size);\
   }

#define BOOST_MATH_DCD_DISTRIBUTION2_TEST(name, param1_table, param2_table, random_variable_table, probability_table) \
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dcd_dist, name), "dist-" BOOST_STRINGIZE(name) "-dcd-cdf")\
   {\
   double result = 0;\
   unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
   unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
   unsigned c_size = sizeof(random_variable_table)/sizeof(random_variable_table[0]);\
   \
   for(unsigned i = 0; i < a_size; ++i)\
   {\
      for(unsigned j = 0; j < b_size; ++j)\
      {\
         for(unsigned k = 0; k < c_size; ++k)\
         {\
            int which = 1;\
            double p; double q; \
            double rv = random_variable_table[k];\
            double a = param1_table[i];\
            double b = param2_table[j];\
            int status = 0;\
            double bound = 0;\
            BOOST_JOIN(cdf, name)(&which, &p, &q, &rv, &a, &b, &status, &bound);\
            result += p;\
            BOOST_ASSERT((status != 0) || check_near(p, cdf(boost::math:: BOOST_JOIN(name, _distribution) <>(param1_table[i], param2_table[j]), random_variable_table[k])));\
         }\
      }\
   }\
   \
   consume_result(result);\
   set_call_count(a_size * b_size * c_size);\
   }\
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dcd_dist_quant, name), "dist-" BOOST_STRINGIZE(name) "-dcd-quantile")\
   {\
      double result = 0;\
      unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
      unsigned b_size = sizeof(param2_table)/sizeof(param2_table[0]);\
      unsigned c_size = sizeof(probability_table)/sizeof(probability_table[0]);\
      \
      for(unsigned i = 0; i < a_size; ++i)\
      {\
         for(unsigned j = 0; j < b_size; ++j)\
         {\
            for(unsigned k = 0; k < c_size; ++k)\
            {\
               int which = 2;\
               double p = probability_table[k];\
               double q = 1 - p; \
               double rv;\
               double a = param1_table[i];\
               double b = param2_table[j];\
               int status = 0;\
               double bound = 0;\
               BOOST_JOIN(cdf, name)(&which, &p, &q, &rv, &a, &b, &status, &bound);\
               result += rv;\
               BOOST_ASSERT((status != 0) || (p > 0.99) || check_near(rv, quantile(boost::math:: BOOST_JOIN(name, _distribution) <>(param1_table[i], param2_table[j]), probability_table[k])));\
            }\
         }\
      }\
      \
      consume_result(result);\
      set_call_count(a_size * b_size * c_size);\
   }

#define BOOST_MATH_DCD_DISTRIBUTION1_TEST(name, param1_table, random_variable_table, probability_table) \
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dcd_dist, name), "dist-" BOOST_STRINGIZE(name) "-dcd-cdf")\
   {\
   double result = 0;\
   unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
   unsigned c_size = sizeof(random_variable_table)/sizeof(random_variable_table[0]);\
   \
   for(unsigned i = 0; i < a_size; ++i)\
   {\
         for(unsigned k = 0; k < c_size; ++k)\
         {\
            int which = 1;\
            double p; double q; \
            double rv = random_variable_table[k];\
            double a = param1_table[i];\
            int status = 0;\
            double bound = 0;\
            BOOST_JOIN(cdf, name)(&which, &p, &q, &rv, &a, &status, &bound);\
            result += p;\
            BOOST_ASSERT((status != 0) || check_near(p, cdf(boost::math:: BOOST_JOIN(name, _distribution) <>(param1_table[i]), random_variable_table[k])));\
         }\
   }\
   \
   consume_result(result);\
   set_call_count(a_size * c_size);\
   }\
   BOOST_MATH_PERFORMANCE_TEST(BOOST_JOIN(dcd_dist_quant, name), "dist-" BOOST_STRINGIZE(name) "-dcd-quantile")\
   {\
      double result = 0;\
      unsigned a_size = sizeof(param1_table)/sizeof(param1_table[0]);\
      unsigned c_size = sizeof(probability_table)/sizeof(probability_table[0]);\
      \
      for(unsigned i = 0; i < a_size; ++i)\
      {\
            for(unsigned k = 0; k < c_size; ++k)\
            {\
               int which = 2;\
               double p = probability_table[k];\
               double q = 1 - p; \
               double rv;\
               double a = param1_table[i];\
               int status = 0;\
               double bound = 0;\
               BOOST_JOIN(cdf, name)(&which, &p, &q, &rv, &a, &status, &bound);\
               result += rv;\
               BOOST_ASSERT((status != 0) || (p > 0.99) || check_near(rv, quantile(boost::math:: BOOST_JOIN(name, _distribution) <>(param1_table[i]), probability_table[k])));\
            }\
      }\
      \
      consume_result(result);\
      set_call_count(a_size * c_size);\
   }

BOOST_MATH_DCD_DISTRIBUTION2_TEST(beta, probabilities, probabilities, probabilities, probabilities) // ??
BOOST_MATH_DCD_DISTRIBUTION2_TEST(binomial, int_values, probabilities, int_values, probabilities) // OK ish
BOOST_MATH_DCD_DISTRIBUTION1_TEST(chi_squared, int_values, real_values, probabilities) // OK
BOOST_MATH_DCD_DISTRIBUTION2_TEST(non_central_chi_squared, int_values, int_values, real_values, probabilities) // Error rates quite high for DCD version?
BOOST_MATH_DCD_DISTRIBUTION2_TEST(f, int_values, int_values, real_values, probabilities) // OK
BOOST_MATH_DCD_DISTRIBUTION3_TEST(fnc, int_values, int_values, real_values, real_values, probabilities) // Error rates quite high for DCD version?
BOOST_MATH_DCD_DISTRIBUTION2_TEST(gam, real_values, real_values, real_values, probabilities) // ??
BOOST_MATH_DCD_DISTRIBUTION2_TEST(negative_binomial, int_values, probabilities, int_values, probabilities) // OK
BOOST_MATH_DCD_DISTRIBUTION2_TEST(nor, real_values, real_values, real_values, probabilities) // OK
BOOST_MATH_DCD_DISTRIBUTION1_TEST(poi, real_values, int_values, probabilities) // OK
BOOST_MATH_DCD_DISTRIBUTION1_TEST(t, int_values, real_values, probabilities) // OK

#endif
