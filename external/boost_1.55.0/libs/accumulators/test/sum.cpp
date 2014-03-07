//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/sum.hpp>
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
    accumulator_set<int, stats<tag::sum, tag::sum_of_weights, tag::sum_of_variates<int, tag::covariate1> >, int> acc;

    acc(1, weight = 2, covariate1 = 3);
    BOOST_CHECK_EQUAL(2, sum(acc));
    BOOST_CHECK_EQUAL(2, sum_of_weights(acc));
    BOOST_CHECK_EQUAL(3, sum_of_variates(acc));

    acc(2, weight = 4, covariate1 = 6);
    BOOST_CHECK_EQUAL(10, sum(acc));
    BOOST_CHECK_EQUAL(6, sum_of_weights(acc));
    BOOST_CHECK_EQUAL(9, sum_of_variates(acc));

    acc(3, weight = 6, covariate1 = 9);
    BOOST_CHECK_EQUAL(28, sum(acc));
    BOOST_CHECK_EQUAL(12, sum_of_weights(acc));
    BOOST_CHECK_EQUAL(18, sum_of_variates(acc));
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("sum test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}
