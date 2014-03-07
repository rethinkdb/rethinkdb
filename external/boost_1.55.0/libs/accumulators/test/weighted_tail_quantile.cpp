//  (C) Copyright 2006 Eric Niebler, Olivier Gygi.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test case for weighted_tail_quantile.hpp

#define BOOST_NUMERIC_FUNCTIONAL_STD_COMPLEX_SUPPORT
#define BOOST_NUMERIC_FUNCTIONAL_STD_VALARRAY_SUPPORT
#define BOOST_NUMERIC_FUNCTIONAL_STD_VECTOR_SUPPORT

#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/weighted_tail_quantile.hpp>

using namespace boost;
using namespace unit_test;
using namespace boost::accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    // tolerance in %
    double epsilon = 1;

    std::size_t n = 100000; // number of MC steps
    std::size_t c =  20000; // cache size

    double mu1 = 1.0;
    double mu2 = -1.0;
    boost::lagged_fibonacci607 rng;
    boost::normal_distribution<> mean_sigma1(mu1,1);
    boost::normal_distribution<> mean_sigma2(mu2,1);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal1(rng, mean_sigma1);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal2(rng, mean_sigma2);

    accumulator_set<double, stats<tag::weighted_tail_quantile<right> >, double>
        acc1(right_tail_cache_size = c);

    accumulator_set<double, stats<tag::weighted_tail_quantile<left> >, double>
        acc2(left_tail_cache_size = c);

    for (std::size_t i = 0; i < n; ++i)
    {
        double sample1 = normal1();
        double sample2 = normal2();
        acc1(sample1, weight = std::exp(-mu1 * (sample1 - 0.5 * mu1)));
        acc2(sample2, weight = std::exp(-mu2 * (sample2 - 0.5 * mu2)));
    }

    // check standard normal distribution
    BOOST_CHECK_CLOSE( quantile(acc1, quantile_probability = 0.975),  1.959963, epsilon );
    BOOST_CHECK_CLOSE( quantile(acc1, quantile_probability = 0.999),  3.090232, epsilon );
    BOOST_CHECK_CLOSE( quantile(acc2, quantile_probability  = 0.025), -1.959963, epsilon );
    BOOST_CHECK_CLOSE( quantile(acc2, quantile_probability  = 0.001), -3.090232, epsilon );

}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("weighted_tail_quantile test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}

