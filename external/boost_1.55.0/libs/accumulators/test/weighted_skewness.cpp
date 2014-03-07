//  (C) Copyright 2006 Eric Niebler, Olivier Gygi.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test case for weighted_skewness.hpp

#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/numeric/functional/vector.hpp>
#include <boost/accumulators/numeric/functional/complex.hpp>
#include <boost/accumulators/numeric/functional/valarray.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/weighted_skewness.hpp>

using namespace boost;
using namespace unit_test;
using namespace boost::accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    // tolerance in %
    // double epsilon = 1;

    accumulator_set<double, stats<tag::weighted_skewness>, double > acc1;
    accumulator_set<int, stats<tag::weighted_skewness>, int > acc2;

    // two random number generators
    boost::lagged_fibonacci607 rng;
    boost::normal_distribution<> mean_sigma(0,1);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal(rng, mean_sigma);

    for (std::size_t i=0; i<100000; ++i)
    {
        acc1(normal(), weight = rng());
    }

    // This check fails because epsilon is relative and not absolute
    // BOOST_CHECK_CLOSE( weighted_skewness(acc1), 0., epsilon );

    acc2(2, weight = 4);
    acc2(7, weight = 1);
    acc2(4, weight = 3);
    acc2(9, weight = 1);
    acc2(3, weight = 2);

    BOOST_CHECK_EQUAL( weighted_mean(acc2), 42./11. );
    BOOST_CHECK_EQUAL( accumulators::weighted_moment<2>(acc2), 212./11. );
    BOOST_CHECK_EQUAL( accumulators::weighted_moment<3>(acc2), 1350./11. );
    BOOST_CHECK_CLOSE( weighted_skewness(acc2), 1.30708406282, 1e-6 );
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("weighted_skewness test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}

