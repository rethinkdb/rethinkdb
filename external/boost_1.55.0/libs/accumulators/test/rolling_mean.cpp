//  (C) Copyright Eric Niebler 2008.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/rolling_mean.hpp>

using namespace boost;
using namespace unit_test;
using namespace accumulators;

template<typename T>
void assert_is_double(T const &)
{
    BOOST_MPL_ASSERT((is_same<T, double>));
}

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    accumulator_set<
        int
      , stats<
            tag::rolling_mean
        >
    > acc(tag::rolling_mean::window_size = 5), test_acc(tag::rolling_mean::window_size = 5, sample = 0);

    acc(1);
    BOOST_CHECK_CLOSE(1., rolling_mean(acc), 1e-5);

    acc(2);
    BOOST_CHECK_CLOSE(1.5, rolling_mean(acc), 1e-5);

    acc(3);
    BOOST_CHECK_CLOSE(2., rolling_mean(acc), 1e-5);

    acc(4);
    BOOST_CHECK_CLOSE(2.5, rolling_mean(acc), 1e-5);

    acc(5);
    BOOST_CHECK_CLOSE(3., rolling_mean(acc), 1e-5);

    acc(6);
    BOOST_CHECK_CLOSE(4., rolling_mean(acc), 1e-5);

    acc(7);
    BOOST_CHECK_CLOSE(5., rolling_mean(acc), 1e-5);

    assert_is_double(rolling_mean(acc));
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("rolling mean test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}
