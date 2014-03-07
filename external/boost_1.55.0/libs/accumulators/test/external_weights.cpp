//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/weighted_mean.hpp>

using namespace boost;
using namespace unit_test;
using namespace accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    accumulator_set<int, stats<tag::weighted_mean>, external<int> > acc;
    accumulator_set<void, stats<tag::sum_of_weights>, int> weight_acc;

    acc(10, weight = 2);                        //  20
    weight_acc(weight = 2);                     //
    BOOST_CHECK_EQUAL(2, sum_of_weights(weight_acc));   //
                                                //
    acc(6, weight = 3);                         //  18
    weight_acc(weight = 3);                     //
    BOOST_CHECK_EQUAL(5, sum_of_weights(weight_acc));   //
                                                //
    acc(4, weight = 4);                         //  16
    weight_acc(weight = 4);                     //
    BOOST_CHECK_EQUAL(9, sum_of_weights(weight_acc));   //
                                                //
    acc(6, weight = 5);                         //+ 30
    weight_acc(weight = 5);                     //
    BOOST_CHECK_EQUAL(14, sum_of_weights(weight_acc));  //
                                                //= 84  / 14 = 6

    BOOST_CHECK_CLOSE(6., weighted_mean(acc, weights = weight_acc), 1e-5);
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("external_weights test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}
