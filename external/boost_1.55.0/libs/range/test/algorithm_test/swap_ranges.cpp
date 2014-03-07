// Boost.Range library
//
//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/algorithm/swap_ranges.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include <algorithm>
#include <functional>
#include <list>
#include <numeric>
#include <deque>
#include <vector>

namespace
{
    template<class Container1, class Container2>
    void test_swap_ranges_impl(const Container1& source1, const Container2& source2)
    {
        Container1 reference1(source1);
        Container2 reference2(source2);
        std::swap_ranges(reference1.begin(), reference1.end(), reference2.begin());

        Container1 test1(source1);
        Container2 test2(source2);
        boost::swap_ranges(test1, test2);

        BOOST_CHECK_EQUAL_COLLECTIONS( reference1.begin(), reference1.end(),
                                       test1.begin(), test1.end() );

        BOOST_CHECK_EQUAL_COLLECTIONS( reference2.begin(), reference2.end(),
                                       test2.begin(), test2.end() );
                                       
        test1 = source1;
        test2 = source2;
        boost::swap_ranges(boost::make_iterator_range(test1), test2);
        BOOST_CHECK_EQUAL_COLLECTIONS( reference1.begin(), reference1.end(),
                                       test1.begin(), test1.end() );
        BOOST_CHECK_EQUAL_COLLECTIONS( reference2.begin(), reference2.end(),
                                       test2.begin(), test2.end() );
                                       
        test1 = source1;
        test2 = source2;
        boost::swap_ranges(test1, boost::make_iterator_range(test2));
        BOOST_CHECK_EQUAL_COLLECTIONS( reference1.begin(), reference1.end(),
                                       test1.begin(), test1.end() );
        BOOST_CHECK_EQUAL_COLLECTIONS( reference2.begin(), reference2.end(),
                                       test2.begin(), test2.end() );
                                       
        test1 = source1;
        test2 = source2;
        boost::swap_ranges(boost::make_iterator_range(test1),
                           boost::make_iterator_range(test2));
        BOOST_CHECK_EQUAL_COLLECTIONS( reference1.begin(), reference1.end(),
                                       test1.begin(), test1.end() );
        BOOST_CHECK_EQUAL_COLLECTIONS( reference2.begin(), reference2.end(),
                                       test2.begin(), test2.end() );
    }

    template<class Container1, class Container2>
    void test_swap_ranges_impl()
    {
        using namespace boost::assign;

        Container1 c1;
        Container2 c2;

        test_swap_ranges_impl(c1, c2);

        c1.clear();
        c1 += 1;
        c2.clear();
        c2 += 2;
        test_swap_ranges_impl(c1, c2);

        c1.clear();
        c1 += 1,2,3,4,5,6,7,8,9,10;
        c2.clear();
        c2 += 10,9,8,7,6,5,4,3,2,1;
        test_swap_ranges_impl(c1, c2);
    }

    inline void test_swap_ranges()
    {
        test_swap_ranges_impl< std::vector<int>, std::vector<int> >();
        test_swap_ranges_impl< std::vector<int>, std::list<int> >();
        test_swap_ranges_impl< std::vector<int>, std::deque<int> >();
        test_swap_ranges_impl< std::list<int>, std::vector<int> >();
        test_swap_ranges_impl< std::list<int>, std::list<int> >();
        test_swap_ranges_impl< std::list<int>, std::deque<int> >();
        test_swap_ranges_impl< std::deque<int>, std::vector<int> >();
        test_swap_ranges_impl< std::deque<int>, std::list<int> >();
        test_swap_ranges_impl< std::deque<int>, std::deque<int> >();
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.swap_ranges" );

    test->add( BOOST_TEST_CASE( &test_swap_ranges ) );

    return test;
}
