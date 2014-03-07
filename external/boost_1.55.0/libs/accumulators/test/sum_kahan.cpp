//  (C) Copyright Gaetano Mendola 2010, Simon West 2011.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/sum_kahan.hpp>
#include <boost/accumulators/statistics/variates/covariate.hpp>

using namespace boost;
using namespace unit_test;
using namespace accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_sum_kahan
//
void test_sum_kahan()
{
    accumulator_set<float, stats<tag::sum_kahan> > acc;

    BOOST_CHECK_EQUAL(0.0f, sum_kahan(acc));

    for (size_t i = 0; i < 1e6; ++i) {
        acc(1e-6f);
    }

    BOOST_CHECK_EQUAL(1.0f, sum_kahan(acc));
}

///////////////////////////////////////////////////////////////////////////////
// test_sum_of_weights_kahan
//
void test_sum_of_weights_kahan()
{
    accumulator_set<float, stats<tag::sum_of_weights_kahan>, float > acc;

    BOOST_CHECK_EQUAL(0.0f, sum_of_weights_kahan(acc));

    for (size_t i = 0; i < 1e6; ++i) {
        acc(0, weight = 1e-6f);
    }

    BOOST_CHECK_EQUAL(1.0f, sum_of_weights_kahan(acc));
}

///////////////////////////////////////////////////////////////////////////////
// test_sum_of_variates_kahan
//
void test_sum_of_variates_kahan()
{
    accumulator_set<
        float, 
        stats<tag::sum_of_variates_kahan<float, tag::covariate1> >,
        float
    >
    acc;

    BOOST_CHECK_EQUAL(0.0f, sum_of_variates_kahan(acc));

    for (size_t i = 0; i < 1e6; ++i) {
        acc(0, covariate1 = 1e-6f);
    }

    BOOST_CHECK_EQUAL(1.0f, sum_of_variates_kahan(acc));
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("sum kahan tests");

    test->add(BOOST_TEST_CASE(&test_sum_kahan));
    test->add(BOOST_TEST_CASE(&test_sum_of_weights_kahan));
    test->add(BOOST_TEST_CASE(&test_sum_of_variates_kahan));

    return test;
}
