// Boost.Range library
//
//  Copyright Neil Groves 2010. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/algorithm_ext/for_each.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/range/iterator.hpp>
#include <algorithm>
#include <list>
#include <vector>

namespace
{
    struct MockBinaryFn
    {
        typedef void result_type;
        typedef int first_argument_type;
        typedef int second_argument_type;

        void operator()(int x, int y)
        {
            xs.push_back(x);
            ys.push_back(y);
        }

        std::vector<int> xs;
        std::vector<int> ys;
    };

    template< class Range1, class Range2 >
    void test_for_each_impl( Range1& rng1, Range2& rng2 )
    {
        MockBinaryFn fn = boost::range::for_each(rng1, rng2, MockBinaryFn());

        BOOST_CHECK_EQUAL_COLLECTIONS( ::boost::begin(rng1), ::boost::end(rng1),
            fn.xs.begin(), fn.xs.end() );

        BOOST_CHECK_EQUAL_COLLECTIONS( ::boost::begin(rng2), ::boost::end(rng2),
            fn.ys.begin(), fn.ys.end() );
    }

    template< class Collection1, class Collection2 >
    void test_for_each_impl(const int max_count)
    {
        Collection1 c1;
        for (int i = 0; i < max_count; ++i)
            c1.push_back(i);

        Collection2 c2;
        for (int i = 0; i < max_count; ++i)
            c2.push_back(i);

        test_for_each_impl(c1, c2);

        const Collection1& const_c1 = c1;
        const Collection2& const_c2 = c2;

        test_for_each_impl(c1, const_c2);
        test_for_each_impl(const_c1, c2);
        test_for_each_impl(const_c1, const_c2);
    }

    template< class Collection1, class Collection2 >
    void test_for_each_impl()
    {
        test_for_each_impl< Collection1, Collection2 >(0);
        test_for_each_impl< Collection1, Collection2 >(1);
        test_for_each_impl< Collection1, Collection2 >(10);
    }

    void test_for_each()
    {
        test_for_each_impl< std::vector<int>, std::vector<int> >();
        test_for_each_impl< std::list<int>, std::list<int> >();
        test_for_each_impl< std::vector<int>, std::list<int> >();
        test_for_each_impl< std::list<int>, std::vector<int> >();
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm_ext.for_each" );

    test->add( BOOST_TEST_CASE( &test_for_each ) );

    return test;
}
