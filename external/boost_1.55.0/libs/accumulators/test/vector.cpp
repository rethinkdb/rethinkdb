//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <vector>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/numeric/functional/vector.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/weighted_mean.hpp>

using namespace boost;
using namespace unit_test;
using namespace accumulators;

template<typename T>
typename enable_if<is_floating_point<T> >::type is_equal_or_close(T const &left, T const &right)
{
    BOOST_CHECK_CLOSE(left, right, 1e-5);
}

template<typename T>
typename disable_if<is_floating_point<T> >::type is_equal_or_close(T const &left, T const &right)
{
    BOOST_CHECK_EQUAL(left, right);
}

template<typename T>
void is_equal(std::vector<T> const &left, std::vector<T> const &right)
{
    BOOST_CHECK_EQUAL(left.size(), right.size());
    if(left.size() == right.size())
    {
        for(std::size_t i = 0; i < left.size(); ++i)
        {
            is_equal_or_close(left[i], right[i]);
        }
    }
}

namespace std
{
    template<typename T>
    inline std::ostream &operator <<(std::ostream &sout, std::vector<T> const &arr)
    {
        sout << '(';
        for(std::size_t i = 0; i < arr.size(); ++i)
        {
            sout << arr[i] << ',';
        }
        sout << ')' << std::endl;
        return sout;
    }
}

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    typedef std::vector<int> sample_t;

    // test sum
    {
        accumulator_set<sample_t, stats<tag::sum> > acc(sample = sample_t(3,0));

        acc(sample_t(3,1));
        acc(sample_t(3,2));
        acc(sample_t(3,3));

        is_equal(sample_t(3,6), sum(acc));
    }

    // test min and max
    {
        int s1[] = {1,2,3}, s2[] = {0,3,4}, s3[] = {2,1,4}, min_res[] = {0,1,3}, max_res[] = {2,3,4};
        accumulator_set<sample_t, stats<tag::min, tag::max> > acc(sample = sample_t(3,0));

        acc(sample_t(s1,s1+3));
        acc(sample_t(s2,s2+3));
        acc(sample_t(s3,s3+3));

        is_equal(sample_t(min_res,min_res+3), (min)(acc));
        is_equal(sample_t(max_res,max_res+3), (max)(acc));
    }

    // test mean(lazy) and mean(immediate)
    {
        accumulator_set<sample_t, stats<tag::mean> > acc(sample = sample_t(3,0));

        acc(sample_t(3,1));
        is_equal(std::vector<double>(3, 1.), mean(acc));
        BOOST_CHECK_EQUAL(1u, count(acc));
        is_equal(sample_t(3,1), sum(acc));

        acc(sample_t(3,0));
        is_equal(std::vector<double>(3, 0.5), mean(acc));
        BOOST_CHECK_EQUAL(2u, count(acc));
        is_equal(sample_t(3,1), sum(acc));

        acc(sample_t(3,2));
        is_equal(std::vector<double>(3, 1.), mean(acc));
        BOOST_CHECK_EQUAL(3u, count(acc));
        is_equal(sample_t(3,3), sum(acc));


        accumulator_set<sample_t, stats<tag::mean(immediate)> > acc2(sample = sample_t(3,0));

        acc2(sample_t(3,1));
        is_equal(std::vector<double>(3,1.), mean(acc2));
        BOOST_CHECK_EQUAL(1u, count(acc2));

        acc2(sample_t(3,0));
        is_equal(std::vector<double>(3,0.5), mean(acc2));
        BOOST_CHECK_EQUAL(2u, count(acc2));

        acc2(sample_t(3,2));
        is_equal(std::vector<double>(3,1.), mean(acc2));
        BOOST_CHECK_EQUAL(3u, count(acc2));
    }

    // test weighted_mean
    {
        accumulator_set<sample_t, stats<tag::weighted_mean>, int> acc(sample = sample_t(3,0));

        acc(sample_t(3,10), weight = 2);            //  20
        BOOST_CHECK_EQUAL(2, sum_of_weights(acc));  //
                                                    //
        acc(sample_t(3,6), weight = 3);             //  18
        BOOST_CHECK_EQUAL(5, sum_of_weights(acc));  //
                                                    //
        acc(sample_t(3,4), weight = 4);             //  16
        BOOST_CHECK_EQUAL(9, sum_of_weights(acc));  //
                                                    //
        acc(sample_t(3,6), weight = 5);             //+ 30
        BOOST_CHECK_EQUAL(14, sum_of_weights(acc)); //
                                                    //= 84  / 14 = 6

        is_equal(std::vector<double>(3,6.), weighted_mean(acc));


        accumulator_set<sample_t, stats<tag::weighted_mean(immediate)>, int> acc2(sample = sample_t(3,0));

        acc2(sample_t(3,10), weight = 2);           //  20
        BOOST_CHECK_EQUAL(2, sum_of_weights(acc2)); //
                                                    //
        acc2(sample_t(3,6), weight = 3);            //  18
        BOOST_CHECK_EQUAL(5, sum_of_weights(acc2)); //
                                                    //
        acc2(sample_t(3,4), weight = 4);            //  16
        BOOST_CHECK_EQUAL(9, sum_of_weights(acc2)); //
                                                    //
        acc2(sample_t(3,6), weight = 5);            //+ 30
        BOOST_CHECK_EQUAL(14, sum_of_weights(acc2));//
                                                    //= 84  / 14 = 6

        is_equal(std::vector<double>(3,6.), weighted_mean(acc2));

    }
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("vector test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}
