//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/with_error.hpp>
#include <boost/accumulators/statistics/error_of_mean.hpp>

using namespace boost;
using namespace unit_test;
using namespace accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    accumulator_set<double, stats<tag::error_of<tag::mean(lazy)> > > acc;
    acc(1.1);
    acc(1.2);
    acc(1.3);
    BOOST_CHECK_CLOSE(0.057735, accumulators::error_of<tag::mean(lazy)>(acc), 1e-4);

    accumulator_set<double, stats<tag::error_of<tag::mean(immediate)> > > acc2;
    acc2(1.1);
    acc2(1.2);
    acc2(1.3);
    BOOST_CHECK_CLOSE(0.057735, accumulators::error_of<tag::mean(immediate)>(acc2), 1e-4);

    accumulator_set<double, stats<with_error<tag::mean(lazy)> > > acc3;
    acc3(1.1);
    acc3(1.2);
    acc3(1.3);
    BOOST_CHECK_CLOSE(0.057735, accumulators::error_of<tag::mean(lazy)>(acc3), 1e-4);

    accumulator_set<double, stats<with_error<tag::mean(immediate)> > > acc4;
    acc4(1.1);
    acc4(1.2);
    acc4(1.3);
    BOOST_CHECK_CLOSE(0.057735, accumulators::error_of<tag::mean(immediate)>(acc4), 1e-4);
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("mean test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}
