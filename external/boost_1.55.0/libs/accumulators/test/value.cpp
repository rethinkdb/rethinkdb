//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>

using namespace boost;
using namespace unit_test;
using namespace accumulators;

namespace my
{
    BOOST_PARAMETER_KEYWORD(tag, int_val)
}

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    int i = 42;
    accumulator_set<double, stats<tag::value<int, my::tag::int_val> > > acc2(
        my::int_val = i);

    int val1 = value<int, my::tag::int_val>(acc2);
    int val2 = value_tag<my::tag::int_val>(acc2);

    BOOST_CHECK_EQUAL(i, val1);
    BOOST_CHECK_EQUAL(i, val2);
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("value_accumulator test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}
