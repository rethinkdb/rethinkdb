//  (C) Copyright 2005 Daniel Egloff, Eric Niebler
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>

using namespace boost;
using namespace unit_test;
using namespace accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    // matlab
    // >> samples = [1:5];
    // >> mean(samples)
    // ans = 3
    // >> sum(samples .* samples) / length(samples)
    // ans = 11
    // >> sum(samples .* samples) / length(samples) - mean(samples)^2
    // ans = 2

    // lazy variance, now lazy with syntactic sugar, thanks to Eric
    accumulator_set<int, stats<tag::variance(lazy)> > acc1;

    acc1(1);
    acc1(2);
    acc1(3);
    acc1(4);
    acc1(5);

    BOOST_CHECK_EQUAL(5u, count(acc1));
    BOOST_CHECK_CLOSE(3., mean(acc1), 1e-5);
    BOOST_CHECK_CLOSE(11., accumulators::moment<2>(acc1), 1e-5);
    BOOST_CHECK_CLOSE(2., variance(acc1), 1e-5);

    // immediate variance
    accumulator_set<int, stats<tag::variance> > acc2;

    acc2(1);
    acc2(2);
    acc2(3);
    acc2(4);
    acc2(5);

    BOOST_CHECK_EQUAL(5u, count(acc2));
    BOOST_CHECK_CLOSE(3., mean(acc2), 1e-5);
    BOOST_CHECK_CLOSE(2., variance(acc2), 1e-5);
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("variance test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}
