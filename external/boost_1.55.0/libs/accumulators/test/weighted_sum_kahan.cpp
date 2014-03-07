//  (C) Copyright Simon West 2011.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/weighted_sum_kahan.hpp>
#include <boost/accumulators/statistics/variates/covariate.hpp>

using namespace boost;
using namespace unit_test;
using namespace accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_weighted_sum_kahan
//
void test_weighted_sum_kahan()
{
    accumulator_set<float, stats<tag::weighted_sum_kahan>, float > acc;

    BOOST_CHECK_EQUAL(0.0f, weighted_sum_kahan(acc));

    for (size_t i = 0; i < 1e6; ++i) {
        acc(1, weight = 1e-6f);
    }

    BOOST_CHECK_EQUAL(1.0f, weighted_sum_kahan(acc));
}

///////////////////////////////////////////////////////////////////////////////
// test_weighted_sum_of_variates_kahan
//
void test_weighted_sum_of_variates_kahan()
{
    accumulator_set<
        float, 
        stats<tag::weighted_sum_of_variates_kahan<float, tag::covariate1> >,
        float
    >
    acc;

    BOOST_CHECK_EQUAL(0.0f, weighted_sum_of_variates_kahan(acc));

    for (size_t i = 0; i < 1e6; ++i) {
      acc(0, weight = 1.0f, covariate1 = 1e-6f);
    }

    BOOST_CHECK_EQUAL(1.0f, weighted_sum_of_variates_kahan(acc));
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("weighted sum kahan tests");

    test->add(BOOST_TEST_CASE(&test_weighted_sum_kahan));
    test->add(BOOST_TEST_CASE(&test_weighted_sum_of_variates_kahan));

    return test;
}
