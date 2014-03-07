//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test case for pot_quantile.hpp

#define BOOST_NUMERIC_FUNCTIONAL_STD_COMPLEX_SUPPORT
#define BOOST_NUMERIC_FUNCTIONAL_STD_VALARRAY_SUPPORT
#define BOOST_NUMERIC_FUNCTIONAL_STD_VECTOR_SUPPORT

#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/peaks_over_threshold.hpp>

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

    // two random number generators
    boost::lagged_fibonacci607 rng;
    boost::normal_distribution<> mean_sigma(0,1);
    boost::exponential_distribution<> lambda(1);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal(rng, mean_sigma);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::exponential_distribution<> > exponential(rng, lambda);

    accumulator_set<double, stats<tag::pot_quantile<right>(with_threshold_value)> > acc1(
        pot_threshold_value = 3.
    );
    accumulator_set<double, stats<tag::pot_quantile<right>(with_threshold_probability)> > acc2(
        right_tail_cache_size = 2000
      , pot_threshold_probability = 0.99
    );
    accumulator_set<double, stats<tag::pot_quantile<left>(with_threshold_value)> > acc3(
        pot_threshold_value = -3.
    );
    accumulator_set<double, stats<tag::pot_quantile<left>(with_threshold_probability)> > acc4(
        left_tail_cache_size = 2000
      , pot_threshold_probability = 0.01
    );

    accumulator_set<double, stats<tag::pot_quantile<right>(with_threshold_value)> > acc5(
        pot_threshold_value = 5.
    );
    accumulator_set<double, stats<tag::pot_quantile<right>(with_threshold_probability)> > acc6(
        right_tail_cache_size = 2000
      , pot_threshold_probability = 0.995
    );

    for (std::size_t i = 0; i < 100000; ++i)
    {
        double sample = normal();
        acc1(sample);
        acc2(sample);
        acc3(sample);
        acc4(sample);
    }

    for (std::size_t i = 0; i < 100000; ++i)
    {
        double sample = exponential();
        acc5(sample);
        acc6(sample);
    }

    BOOST_CHECK_CLOSE( quantile(acc1, quantile_probability = 0.999), 3.090232, 3*epsilon );
    BOOST_CHECK_CLOSE( quantile(acc2, quantile_probability = 0.999), 3.090232, 2*epsilon );
    BOOST_CHECK_CLOSE( quantile(acc3, quantile_probability = 0.001), -3.090232, 2*epsilon );
    BOOST_CHECK_CLOSE( quantile(acc4, quantile_probability = 0.001), -3.090232, 2*epsilon );

    BOOST_CHECK_CLOSE( quantile(acc5, quantile_probability = 0.999), 6.908, 3*epsilon );
    BOOST_CHECK_CLOSE( quantile(acc6, quantile_probability = 0.999), 6.908, 3*epsilon );
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("pot_quantile test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}

