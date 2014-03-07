//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>

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
    accumulator_set<int, stats<droppable<tag::mean> > > acc, test_acc(sample = 0);

    acc(1);
    BOOST_CHECK_CLOSE(1., mean(acc), 1e-5);
    BOOST_CHECK_EQUAL(1u, count(acc));
    BOOST_CHECK_EQUAL(1, sum(acc));

    acc(0);
    BOOST_CHECK_CLOSE(0.5, mean(acc), 1e-5);
    BOOST_CHECK_EQUAL(2u, count(acc));
    BOOST_CHECK_EQUAL(1, sum(acc));

    acc.drop<tag::mean>();

    acc(2);
    BOOST_CHECK_CLOSE(0.5, mean(acc), 1e-5);
    BOOST_CHECK_EQUAL(2u, count(acc));
    BOOST_CHECK_EQUAL(1, sum(acc));

    assert_is_double(mean(acc));


    accumulator_set<int, stats<droppable<tag::mean(immediate)> > > acc2, test_acc2(sample = 0);

    acc2(1);
    BOOST_CHECK_CLOSE(1., mean(acc2), 1e-5);
    BOOST_CHECK_EQUAL(1u, count(acc2));

    acc2(0);
    BOOST_CHECK_CLOSE(0.5, mean(acc2), 1e-5);
    BOOST_CHECK_EQUAL(2u, count(acc2));

    acc2.drop<tag::mean>();

    acc2(2);
    BOOST_CHECK_CLOSE(0.5, mean(acc2), 1e-5);
    BOOST_CHECK_EQUAL(2u, count(acc2));

    assert_is_double(mean(acc2));
}

///////////////////////////////////////////////////////////////////////////////
// test_stat2
//
void test_stat2()
{
    accumulator_set<int, stats<droppable<tag::sum>, droppable<tag::mean> > > acc, test_acc(sample = 0);

    acc(1);
    BOOST_CHECK_CLOSE(1., mean(acc), 1e-5);
    BOOST_CHECK_EQUAL(1u, count(acc));
    BOOST_CHECK_EQUAL(1, sum(acc));

    acc(0);
    BOOST_CHECK_CLOSE(0.5, mean(acc), 1e-5);
    BOOST_CHECK_EQUAL(2u, count(acc));
    BOOST_CHECK_EQUAL(1, sum(acc));

    acc.drop<tag::mean>();
    acc.drop<tag::sum>();

    acc(2);
    BOOST_CHECK_CLOSE(0.5, mean(acc), 1e-5);
    BOOST_CHECK_EQUAL(2u, count(acc));
    BOOST_CHECK_EQUAL(1, sum(acc));

    assert_is_double(mean(acc));
}

///////////////////////////////////////////////////////////////////////////////
// test_stat3
//
void test_stat3()
{
    accumulator_set<int, stats<droppable<tag::sum>, droppable<tag::count>, droppable<tag::mean> > > acc, test_acc(sample = 0);

    acc(1);
    BOOST_CHECK_CLOSE(1., mean(acc), 1e-5);
    BOOST_CHECK_EQUAL(1u, count(acc));
    BOOST_CHECK_EQUAL(1, sum(acc));

    acc(0);
    BOOST_CHECK_CLOSE(0.5, mean(acc), 1e-5);
    BOOST_CHECK_EQUAL(2u, count(acc));
    BOOST_CHECK_EQUAL(1, sum(acc));

    acc.drop<tag::mean>();
    acc.drop<tag::sum>();

    acc(2);
    BOOST_CHECK_CLOSE(1./3., mean(acc), 1e-5);
    BOOST_CHECK_EQUAL(3u, count(acc));
    BOOST_CHECK_EQUAL(1, sum(acc));

    acc.drop<tag::count>();
    acc(3);
    BOOST_CHECK_CLOSE(1./3., mean(acc), 1e-5);
    BOOST_CHECK_EQUAL(3u, count(acc));
    BOOST_CHECK_EQUAL(1, sum(acc));

    assert_is_double(mean(acc));
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("droppable test");

    test->add(BOOST_TEST_CASE(&test_stat));
    test->add(BOOST_TEST_CASE(&test_stat2));
    test->add(BOOST_TEST_CASE(&test_stat3));

    return test;
}
