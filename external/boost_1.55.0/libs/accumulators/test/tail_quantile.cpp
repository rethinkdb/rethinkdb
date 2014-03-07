//  (C) Copyright 2006 Eric Niebler, Olivier Gygi.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test case for tail_quantile.hpp

#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/numeric/functional/vector.hpp>
#include <boost/accumulators/numeric/functional/complex.hpp>
#include <boost/accumulators/numeric/functional/valarray.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/tail_quantile.hpp>

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
    std::size_t c =  10000; // cache size

    typedef accumulator_set<double, stats<tag::tail_quantile<right> > > accumulator_t_right;
    typedef accumulator_set<double, stats<tag::tail_quantile<left> > > accumulator_t_left;

    accumulator_t_right acc0( right_tail_cache_size = c );
    accumulator_t_right acc1( right_tail_cache_size = c );
    accumulator_t_left  acc2( left_tail_cache_size = c );
    accumulator_t_left  acc3( left_tail_cache_size = c );

    // two random number generators
    boost::lagged_fibonacci607 rng;
    boost::normal_distribution<> mean_sigma(0,1);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal(rng, mean_sigma);

    for (std::size_t i = 0; i < n; ++i)
    {
        double sample1 = rng();
        double sample2 = normal();
        acc0(sample1);
        acc1(sample2);
        acc2(sample1);
        acc3(sample2);
    }

    // check uniform distribution
    BOOST_CHECK_CLOSE( quantile(acc0, quantile_probability = 0.95 ), 0.95,  epsilon );
    BOOST_CHECK_CLOSE( quantile(acc0, quantile_probability = 0.975), 0.975, epsilon );
    BOOST_CHECK_CLOSE( quantile(acc0, quantile_probability = 0.99 ), 0.99,  epsilon );
    BOOST_CHECK_CLOSE( quantile(acc0, quantile_probability = 0.999), 0.999, epsilon );
    BOOST_CHECK_CLOSE( quantile(acc2, quantile_probability  = 0.05 ), 0.05,  4*epsilon );
    BOOST_CHECK_CLOSE( quantile(acc2, quantile_probability  = 0.025), 0.025, 5*epsilon );
    BOOST_CHECK_CLOSE( quantile(acc2, quantile_probability  = 0.01 ), 0.01,  7*epsilon );
    BOOST_CHECK_CLOSE( quantile(acc2, quantile_probability  = 0.001), 0.001, 22*epsilon );

    // check standard normal distribution
    BOOST_CHECK_CLOSE( quantile(acc1, quantile_probability = 0.975),  1.959963, epsilon );
    BOOST_CHECK_CLOSE( quantile(acc1, quantile_probability = 0.999),  3.090232, 3*epsilon );
    BOOST_CHECK_CLOSE( quantile(acc3, quantile_probability  = 0.025), -1.959963, 2*epsilon );
    BOOST_CHECK_CLOSE( quantile(acc3, quantile_probability  = 0.001), -3.090232, 3*epsilon );

}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("tail_quantile test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}

