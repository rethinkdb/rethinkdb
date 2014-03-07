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

namespace my
{
    BOOST_PARAMETER_KEYWORD(tag, sum_acc)
}

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    accumulator_set<int, stats<tag::mean, tag::external<tag::sum, my::tag::sum_acc> > > acc;
    accumulator_set<int, stats<tag::sum> > sum_acc;

    acc(1);
    sum_acc(1);
    BOOST_CHECK_CLOSE(1., mean(acc, my::sum_acc = sum_acc), 1e-5);
    BOOST_CHECK_EQUAL(1u, count(acc));
    BOOST_CHECK_EQUAL(1, sum(sum_acc));

    acc(0);
    sum_acc(0);
    BOOST_CHECK_CLOSE(0.5, mean(acc, my::sum_acc = sum_acc), 1e-5);
    BOOST_CHECK_EQUAL(2u, count(acc));
    BOOST_CHECK_EQUAL(1, sum(acc, my::sum_acc = sum_acc));

    acc(2);
    sum_acc(2);
    BOOST_CHECK_CLOSE(1., mean(acc, my::sum_acc = sum_acc), 1e-5);
    BOOST_CHECK_EQUAL(3u, count(acc));
    BOOST_CHECK_EQUAL(3, sum(acc, my::sum_acc = sum_acc));
}

///////////////////////////////////////////////////////////////////////////////
// test_reference
//
void test_reference()
{
    typedef accumulator_set<int, stats<tag::sum> > sum_acc_type;
    sum_acc_type sum_acc;
    accumulator_set<
        int
      , stats<
            tag::mean
          , tag::external<tag::sum, my::tag::sum_acc>
          , tag::reference<sum_acc_type, my::tag::sum_acc>
        >
    > acc(my::sum_acc = sum_acc);

    acc(1);
    sum_acc(1);
    BOOST_CHECK_CLOSE(1., mean(acc), 1e-5);
    BOOST_CHECK_EQUAL(1u, count(acc));
    BOOST_CHECK_EQUAL(1, sum(sum_acc));

    acc(0);
    sum_acc(0);
    BOOST_CHECK_CLOSE(0.5, mean(acc), 1e-5);
    BOOST_CHECK_EQUAL(2u, count(acc));
    BOOST_CHECK_EQUAL(1, sum(acc));

    acc(2);
    sum_acc(2);
    BOOST_CHECK_CLOSE(1., mean(acc), 1e-5);
    BOOST_CHECK_EQUAL(3u, count(acc));
    BOOST_CHECK_EQUAL(3, sum(acc));
}

///////////////////////////////////////////////////////////////////////////////
// test_reference2
//
void test_reference2()
{
    typedef accumulator_set<int, stats<tag::sum> > sum_acc_type;
    sum_acc_type sum_acc;
    accumulator_set<
        int
      , stats<
            tag::mean
          , tag::external<tag::sum, my::tag::sum_acc, sum_acc_type>
        >
    > acc(my::sum_acc = sum_acc);

    acc(1);
    sum_acc(1);
    BOOST_CHECK_CLOSE(1., mean(acc), 1e-5);
    BOOST_CHECK_EQUAL(1u, count(acc));
    BOOST_CHECK_EQUAL(1, sum(sum_acc));

    acc(0);
    sum_acc(0);
    BOOST_CHECK_CLOSE(0.5, mean(acc), 1e-5);
    BOOST_CHECK_EQUAL(2u, count(acc));
    BOOST_CHECK_EQUAL(1, sum(acc));

    acc(2);
    sum_acc(2);
    BOOST_CHECK_CLOSE(1., mean(acc), 1e-5);
    BOOST_CHECK_EQUAL(3u, count(acc));
    BOOST_CHECK_EQUAL(3, sum(acc));
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("external_accumulator test");

    test->add(BOOST_TEST_CASE(&test_stat));
    test->add(BOOST_TEST_CASE(&test_reference));
    test->add(BOOST_TEST_CASE(&test_reference2));

    return test;
}
