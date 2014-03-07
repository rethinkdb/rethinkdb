//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test case for pot_quantile.hpp (weighted feature)

#define BOOST_NUMERIC_FUNCTIONAL_STD_COMPLEX_SUPPORT
#define BOOST_NUMERIC_FUNCTIONAL_STD_VALARRAY_SUPPORT
#define BOOST_NUMERIC_FUNCTIONAL_STD_VECTOR_SUPPORT

#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>

using namespace boost;
using namespace unit_test;
using namespace boost::accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    // tolerance in %
    double epsilon = 1.;

    double mu1, mu2, l;

    mu1 = 1.;
    mu2 = -1.;
    l = 0.5;

    // two random number generators
    boost::lagged_fibonacci607 rng;
    boost::normal_distribution<> mean_sigma1(mu1,1);
    boost::normal_distribution<> mean_sigma2(mu2,1);
    boost::exponential_distribution<> lambda(l);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal1(rng, mean_sigma1);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal2(rng, mean_sigma2);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::exponential_distribution<> > exponential(rng, lambda);

    accumulator_set<double, stats<tag::weighted_pot_quantile<right>(with_threshold_value)>, double > acc1(
        pot_threshold_value = 3.
    );
    accumulator_set<double, stats<tag::weighted_pot_quantile<right>(with_threshold_probability)>, double > acc2(
        right_tail_cache_size = 10000
      , pot_threshold_probability = 0.99
    );
    accumulator_set<double, stats<tag::weighted_pot_quantile<left>(with_threshold_value)>, double > acc3(
        pot_threshold_value = -3.
    );
    accumulator_set<double, stats<tag::weighted_pot_quantile<left>(with_threshold_probability)>, double > acc4(
        left_tail_cache_size = 10000
      , pot_threshold_probability = 0.01
    );

    accumulator_set<double, stats<tag::weighted_pot_quantile<right>(with_threshold_value)>, double > acc5(
        pot_threshold_value = 5.
    );
    accumulator_set<double, stats<tag::weighted_pot_quantile<right>(with_threshold_probability)>, double > acc6(
        right_tail_cache_size = 10000
      , pot_threshold_probability = 0.995
    );

    for (std::size_t i = 0; i < 100000; ++i)
    {
        double sample1 = normal1();
        double sample2 = normal2();
        acc1(sample1, weight = std::exp(-mu1 * (sample1 - 0.5 * mu1)));
        acc2(sample1, weight = std::exp(-mu1 * (sample1 - 0.5 * mu1)));
        acc3(sample2, weight = std::exp(-mu2 * (sample2 - 0.5 * mu2)));
        acc4(sample2, weight = std::exp(-mu2 * (sample2 - 0.5 * mu2)));
    }

    for (std::size_t i = 0; i < 100000; ++i)
    {
        double sample = exponential();
        acc5(sample, weight = 1./l * std::exp(-sample * (1. - l)));
        acc6(sample, weight = 1./l * std::exp(-sample * (1. - l)));
    }

    BOOST_CHECK_CLOSE( quantile(acc1, quantile_probability = 0.999), 3.090232, epsilon );
    BOOST_CHECK_CLOSE( quantile(acc2, quantile_probability = 0.999), 3.090232, epsilon );
    BOOST_CHECK_CLOSE( quantile(acc3, quantile_probability = 0.001), -3.090232, epsilon );
    BOOST_CHECK_CLOSE( quantile(acc4, quantile_probability = 0.001), -3.090232, epsilon );

    BOOST_CHECK_CLOSE( quantile(acc5, quantile_probability = 0.999), 6.908, epsilon );
    BOOST_CHECK_CLOSE( quantile(acc6, quantile_probability = 0.999), 6.908, epsilon );
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("weighted_pot_quantile test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}
