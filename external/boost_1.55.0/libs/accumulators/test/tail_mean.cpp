//  (C) Copyright 2006 Eric Niebler, Olivier Gygi.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test case for tail_mean.hpp

#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/numeric/functional/vector.hpp>
#include <boost/accumulators/numeric/functional/complex.hpp>
#include <boost/accumulators/numeric/functional/valarray.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/tail_mean.hpp>
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

    typedef accumulator_set<double, stats<tag::non_coherent_tail_mean<right>, tag::tail_quantile<right> > > accumulator_t_right1;
    typedef accumulator_set<double, stats<tag::non_coherent_tail_mean<left>, tag::tail_quantile<left> > > accumulator_t_left1;
    typedef accumulator_set<double, stats<tag::coherent_tail_mean<right>, tag::tail_quantile<right> > > accumulator_t_right2;
    typedef accumulator_set<double, stats<tag::coherent_tail_mean<left>, tag::tail_quantile<left> > > accumulator_t_left2;

    accumulator_t_right1 acc0( right_tail_cache_size = c );
    accumulator_t_left1 acc1( left_tail_cache_size = c );
    accumulator_t_right2 acc2( right_tail_cache_size = c );
    accumulator_t_left2 acc3( left_tail_cache_size = c );

    // a random number generator
    boost::lagged_fibonacci607 rng;

    for (std::size_t i = 0; i < n; ++i)
    {
        double sample = rng();
        acc0(sample);
        acc1(sample);
        acc2(sample);
        acc3(sample);
    }

    // check uniform distribution
    BOOST_CHECK_CLOSE( non_coherent_tail_mean(acc0, quantile_probability = 0.95), 0.975, epsilon );
    BOOST_CHECK_CLOSE( non_coherent_tail_mean(acc0, quantile_probability = 0.975), 0.9875, epsilon );
    BOOST_CHECK_CLOSE( non_coherent_tail_mean(acc0, quantile_probability = 0.99), 0.995, epsilon );
    BOOST_CHECK_CLOSE( non_coherent_tail_mean(acc0, quantile_probability = 0.999), 0.9995, epsilon );
    BOOST_CHECK_CLOSE( non_coherent_tail_mean(acc1, quantile_probability = 0.05), 0.025, 5*epsilon );
    BOOST_CHECK_CLOSE( non_coherent_tail_mean(acc1, quantile_probability = 0.025), 0.0125, 6*epsilon );
    BOOST_CHECK_CLOSE( non_coherent_tail_mean(acc1, quantile_probability = 0.01), 0.005, 8*epsilon );
    BOOST_CHECK_CLOSE( non_coherent_tail_mean(acc1, quantile_probability = 0.001), 0.0005, 25*epsilon );
    BOOST_CHECK_CLOSE( tail_mean(acc2, quantile_probability = 0.95), 0.975, epsilon );
    BOOST_CHECK_CLOSE( tail_mean(acc2, quantile_probability = 0.975), 0.9875, epsilon );
    BOOST_CHECK_CLOSE( tail_mean(acc2, quantile_probability = 0.99), 0.995, epsilon );
    BOOST_CHECK_CLOSE( tail_mean(acc2, quantile_probability = 0.999), 0.9995, epsilon );
    BOOST_CHECK_CLOSE( tail_mean(acc3, quantile_probability = 0.05), 0.025, 5*epsilon );
    BOOST_CHECK_CLOSE( tail_mean(acc3, quantile_probability = 0.025), 0.0125, 6*epsilon );
    BOOST_CHECK_CLOSE( tail_mean(acc3, quantile_probability = 0.01), 0.005, 8*epsilon );
    BOOST_CHECK_CLOSE( tail_mean(acc3, quantile_probability = 0.001), 0.0005, 25*epsilon );
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("tail_mean test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}

