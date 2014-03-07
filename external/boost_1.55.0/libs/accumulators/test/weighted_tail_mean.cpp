//  (C) Copyright 2006 Eric Niebler, Olivier Gygi.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test case for weighted_tail_mean.hpp

#define BOOST_NUMERIC_FUNCTIONAL_STD_COMPLEX_SUPPORT
#define BOOST_NUMERIC_FUNCTIONAL_STD_VALARRAY_SUPPORT
#define BOOST_NUMERIC_FUNCTIONAL_STD_VECTOR_SUPPORT

#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/weighted_tail_mean.hpp>
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
    std::size_t c = 25000; // cache size

    accumulator_set<double, stats<tag::non_coherent_weighted_tail_mean<right> >, double >
        acc0( right_tail_cache_size = c );
    accumulator_set<double, stats<tag::non_coherent_weighted_tail_mean<left> >, double >
        acc1( left_tail_cache_size = c );

    // random number generators
    boost::lagged_fibonacci607 rng;

    for (std::size_t i = 0; i < n; ++i)
    {
        double smpl = std::sqrt(rng());
        acc0(smpl, weight = 1./smpl);
    }

    for (std::size_t i = 0; i < n; ++i)
    {
        double smpl = rng();
        acc1(smpl*smpl, weight = smpl);
    }

    // check uniform distribution
    BOOST_CHECK_CLOSE( non_coherent_weighted_tail_mean(acc0, quantile_probability = 0.95), 0.975, epsilon );
    BOOST_CHECK_CLOSE( non_coherent_weighted_tail_mean(acc0, quantile_probability = 0.975), 0.9875, epsilon );
    BOOST_CHECK_CLOSE( non_coherent_weighted_tail_mean(acc0, quantile_probability = 0.99), 0.995, epsilon );
    BOOST_CHECK_CLOSE( non_coherent_weighted_tail_mean(acc0, quantile_probability = 0.999), 0.9995, epsilon );
    BOOST_CHECK_CLOSE( non_coherent_weighted_tail_mean(acc1, quantile_probability = 0.05), 0.025, epsilon );
    BOOST_CHECK_CLOSE( non_coherent_weighted_tail_mean(acc1, quantile_probability = 0.025), 0.0125, epsilon );
    BOOST_CHECK_CLOSE( non_coherent_weighted_tail_mean(acc1, quantile_probability = 0.01), 0.005, epsilon );
    BOOST_CHECK_CLOSE( non_coherent_weighted_tail_mean(acc1, quantile_probability = 0.001), 0.0005, 5*epsilon );
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("weighted_tail_mean test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}

