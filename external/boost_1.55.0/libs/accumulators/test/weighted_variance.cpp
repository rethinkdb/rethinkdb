//  (C) Copyright 2006 Eric Niebler, Olivier Gygi
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/random.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/weighted_variance.hpp>

using namespace boost;
using namespace unit_test;
using namespace accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    // lazy weighted_variance
    accumulator_set<int, stats<tag::weighted_variance(lazy)>, int> acc1;

    acc1(1, weight = 2);    //  2
    acc1(2, weight = 3);    //  6
    acc1(3, weight = 1);    //  3
    acc1(4, weight = 4);    // 16
    acc1(5, weight = 1);    //  5

    // weighted_mean = (2+6+3+16+5) / (2+3+1+4+1) = 32 / 11 = 2.9090909090909090909090909090909

    BOOST_CHECK_EQUAL(5u, count(acc1));
    BOOST_CHECK_CLOSE(2.9090909, weighted_mean(acc1), 1e-5);
    BOOST_CHECK_CLOSE(10.1818182, accumulators::weighted_moment<2>(acc1), 1e-5);
    BOOST_CHECK_CLOSE(1.7190083, weighted_variance(acc1), 1e-5);

    accumulator_set<int, stats<tag::weighted_variance>, int> acc2;

    acc2(1, weight = 2);
    acc2(2, weight = 3);
    acc2(3, weight = 1);
    acc2(4, weight = 4);
    acc2(5, weight = 1);

    BOOST_CHECK_EQUAL(5u, count(acc2));
    BOOST_CHECK_CLOSE(2.9090909, weighted_mean(acc2), 1e-5);
    BOOST_CHECK_CLOSE(1.7190083, weighted_variance(acc2), 1e-5);

    // check lazy and immediate variance with random numbers

    // two random number generators
    boost::lagged_fibonacci607 rng;
    boost::normal_distribution<> mean_sigma(0,1);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal(rng, mean_sigma);

    accumulator_set<double, stats<tag::weighted_variance(lazy)>, double > acc_lazy;
    accumulator_set<double, stats<tag::weighted_variance>, double > acc_immediate;

    for (std::size_t i=0; i<10000; ++i)
    {
        double value = normal();
        acc_lazy(value, weight = rng());
        acc_immediate(value, weight = rng());
    }

    BOOST_CHECK_CLOSE(1., weighted_variance(acc_lazy), 5.);
    BOOST_CHECK_CLOSE(1., weighted_variance(acc_immediate), 5.);
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("weighted_variance test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}
