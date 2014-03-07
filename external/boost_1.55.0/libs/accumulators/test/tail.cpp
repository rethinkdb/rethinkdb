//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/tail.hpp>
#include <boost/accumulators/statistics/tail_variate.hpp>
#include <boost/accumulators/statistics/variates/covariate.hpp>

using namespace boost;
using namespace unit_test;
using namespace accumulators;

template<typename Range>
void check_tail(Range const &rng, char const *expected)
{
    BOOST_FOREACH(int i, rng)
    {
        if(!*expected)
        {
            BOOST_CHECK(false);
            return;
        }
        BOOST_CHECK_EQUAL(i, *expected++);
    }
    BOOST_CHECK(!*expected);
}

///////////////////////////////////////////////////////////////////////////////
// test_right_tail
//
void test_right_tail()
{
    accumulator_set<int, stats<tag::tail_weights<right>, tag::tail_variate<int, tag::covariate1, right> >, int > acc(
        right_tail_cache_size = 4
    );

    acc(010, weight = 2, covariate1 = 3);
    check_tail(tail(acc), "\10");
    check_tail(tail_variate(acc), "\3");
    check_tail(tail_weights(acc), "\2");

    acc(020, weight = 7, covariate1 = 1);
    check_tail(tail(acc), "\20\10");
    check_tail(tail_variate(acc), "\1\3");
    check_tail(tail_weights(acc), "\7\2");

    acc(014, weight = 6, covariate1 = 4);
    check_tail(tail(acc), "\20\14\10");
    check_tail(tail_variate(acc), "\1\4\3");
    check_tail(tail_weights(acc), "\7\6\2");

    acc(030, weight = 4, covariate1 = 5);
    check_tail(tail(acc), "\30\20\14\10");
    check_tail(tail_variate(acc), "\5\1\4\3");
    check_tail(tail_weights(acc), "\4\7\6\2");

    acc(001, weight = 1, covariate1 = 9);
    check_tail(tail(acc), "\30\20\14\10");
    check_tail(tail_variate(acc), "\5\1\4\3");
    check_tail(tail_weights(acc), "\4\7\6\2");

    acc(011, weight = 3, covariate1 = 7);
    check_tail(tail(acc), "\30\20\14\11");
    check_tail(tail_variate(acc), "\5\1\4\7");
    check_tail(tail_weights(acc), "\4\7\6\3");

}

///////////////////////////////////////////////////////////////////////////////
// test_left_tail
//
void test_left_tail()
{
    accumulator_set<int, stats<tag::tail_weights<left>, tag::tail_variate<int, tag::covariate1, left> >, int > acc(
        left_tail_cache_size = 4
    );

    acc(010, weight = 2, covariate1 = 3);
    check_tail(tail(acc), "\10");
    check_tail(tail_variate(acc), "\3");
    check_tail(tail_weights(acc), "\2");

    acc(020, weight = 7, covariate1 = 1);
    check_tail(tail(acc), "\10\20");
    check_tail(tail_variate(acc), "\3\1");
    check_tail(tail_weights(acc), "\2\7");

    acc(014, weight = 6, covariate1 = 4);
    check_tail(tail(acc), "\10\14\20");
    check_tail(tail_variate(acc), "\3\4\1");
    check_tail(tail_weights(acc), "\2\6\7");

    acc(030, weight = 4, covariate1 = 5);
    check_tail(tail(acc), "\10\14\20\30");
    check_tail(tail_variate(acc), "\3\4\1\5");
    check_tail(tail_weights(acc), "\2\6\7\4");

    acc(001, weight = 1, covariate1 = 9);
    check_tail(tail(acc), "\1\10\14\20");
    check_tail(tail_variate(acc), "\x9\3\4\1");
    check_tail(tail_weights(acc), "\1\2\6\7");

    acc(011, weight = 3, covariate1 = 7);
    check_tail(tail(acc), "\1\10\11\14");
    check_tail(tail_variate(acc), "\x9\3\7\4");
    check_tail(tail_weights(acc), "\1\2\3\6");
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("tail test");

    test->add(BOOST_TEST_CASE(&test_right_tail));
    test->add(BOOST_TEST_CASE(&test_left_tail));

    return test;
}

