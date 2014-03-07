//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>

using namespace boost;
using namespace unit_test;
using namespace accumulators;

namespace my
{
    BOOST_PARAMETER_KEYWORD(tag, int_ref)
    BOOST_PARAMETER_KEYWORD(tag, sum_acc)
}

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    int i = 0;
    accumulator_set<double, stats<tag::reference<int, my::tag::int_ref> > > acc(
        my::int_ref = i);

    int &ref1 = accumulators::reference<int, my::tag::int_ref>(acc);
    int &ref2 = accumulators::reference_tag<my::tag::int_ref>(acc);

    BOOST_CHECK_EQUAL(&i, &ref1);
    BOOST_CHECK_EQUAL(&i, &ref2);
}

///////////////////////////////////////////////////////////////////////////////
// test_external
//
void test_external()
{
    typedef accumulator_set<int, stats<tag::sum> > sum_acc_type;
    sum_acc_type sum_acc; // the sum accumulator
    accumulator_set<
        int
      , stats<
            tag::mean
          , tag::external<tag::sum, my::tag::sum_acc>       // make sum external
          , tag::reference<sum_acc_type, my::tag::sum_acc>  // and hold a reference to it
        >
    > acc_with_ref(my::sum_acc = sum_acc); // initialize the reference sum

    sum_acc(1);
    sum_acc(2); // sum is now 3 for both

    BOOST_CHECK_EQUAL(sum(acc_with_ref), sum(sum_acc));
    BOOST_CHECK_EQUAL(sum(acc_with_ref), 3);
}

///////////////////////////////////////////////////////////////////////////////
// test_external2
//
void test_external2()
{
    typedef accumulator_set<int, stats<tag::sum> > sum_acc_type;
    sum_acc_type sum_acc; // the sum accumulator
    accumulator_set<
        int
      , stats<
            tag::mean
            // make sum external and hold a reference to it
          , tag::external<tag::sum, my::tag::sum_acc, sum_acc_type>
        >
    > acc_with_ref(my::sum_acc = sum_acc); // initialize the reference sum

    sum_acc(1);
    sum_acc(2); // sum is now 3 for both

    BOOST_CHECK_EQUAL(sum(acc_with_ref), sum(sum_acc));
    BOOST_CHECK_EQUAL(sum(acc_with_ref), 3);
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("reference_accumulator test");

    test->add(BOOST_TEST_CASE(&test_stat));
    test->add(BOOST_TEST_CASE(&test_external));
    test->add(BOOST_TEST_CASE(&test_external2));

    return test;
}
