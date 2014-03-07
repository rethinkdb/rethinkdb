//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/rolling_count.hpp>

using namespace boost;
using namespace unit_test;
using namespace accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    accumulator_set<int, stats<tag::rolling_count> > acc(tag::rolling_window::window_size = 3);

    BOOST_CHECK_EQUAL(0u, rolling_count(acc));

    acc(1);
    BOOST_CHECK_EQUAL(1u, rolling_count(acc));

    acc(1);
    BOOST_CHECK_EQUAL(2u, rolling_count(acc));

    acc(1);
    BOOST_CHECK_EQUAL(3u, rolling_count(acc));

    acc(1);
    BOOST_CHECK_EQUAL(3u, rolling_count(acc));

    acc(1);
    BOOST_CHECK_EQUAL(3u, rolling_count(acc));
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("rolling count test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}
