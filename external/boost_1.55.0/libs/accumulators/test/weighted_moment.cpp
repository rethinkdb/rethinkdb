//  (C) Copyright Eric Niebler, Olivier Gygi 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/weighted_moment.hpp>

using namespace boost;
using namespace unit_test;
using namespace accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    accumulator_set<double, stats<tag::weighted_moment<2> >, double> acc2;
    accumulator_set<double, stats<tag::weighted_moment<7> >, double> acc7;

    acc2(2.1, weight = 0.7);
    acc2(2.7, weight = 1.4);
    acc2(1.8, weight = 0.9);

    acc7(2.1, weight = 0.7);
    acc7(2.7, weight = 1.4);
    acc7(1.8, weight = 0.9);

    BOOST_CHECK_CLOSE(5.403, accumulators::weighted_moment<2>(acc2), 1e-5);
    BOOST_CHECK_CLOSE(548.54182, accumulators::weighted_moment<7>(acc7), 1e-5);
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("weighted_moment test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}

