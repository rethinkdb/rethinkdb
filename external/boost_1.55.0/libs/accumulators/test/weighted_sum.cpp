//  (C) Copyright Eric Niebler, Olivier Gygi 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/weighted_sum.hpp>
#include <boost/accumulators/statistics/variates/covariate.hpp>

using namespace boost;
using namespace unit_test;
using namespace accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    accumulator_set<int, stats<tag::weighted_sum, tag::weighted_sum_of_variates<int, tag::covariate1> >, int> acc;

    acc(1, weight = 2, covariate1 = 3);
    BOOST_CHECK_EQUAL(2, weighted_sum(acc));
    BOOST_CHECK_EQUAL(6, weighted_sum_of_variates(acc));

    acc(2, weight = 3, covariate1 = 6);
    BOOST_CHECK_EQUAL(8, weighted_sum(acc));
    BOOST_CHECK_EQUAL(24, weighted_sum_of_variates(acc));

    acc(4, weight = 6, covariate1 = 9);
    BOOST_CHECK_EQUAL(32, weighted_sum(acc));
    BOOST_CHECK_EQUAL(78, weighted_sum_of_variates(acc));
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("weighted_sum test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}
