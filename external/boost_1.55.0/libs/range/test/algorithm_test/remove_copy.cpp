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
#include <boost/range/algorithm/remove_copy.hpp>

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
    template<typename Iterator, typename Value>
    void test_append(Iterator target, Value value)
    {
        *target++ = value;
    }

    template< class Container, class Value >
    void test_remove_copy_impl( const Container& c, Value to_remove )
    {
        typedef typename boost::range_value<Container>::type value_type;
        std::vector<value_type> reference;

        typedef BOOST_DEDUCED_TYPENAME std::vector<value_type>::iterator iterator_t;

        test_append(
            std::remove_copy(c.begin(), c.end(),
                                std::back_inserter(reference), to_remove),
            to_remove);

        std::vector<value_type> test;
        test_append(
            boost::remove_copy(c, std::back_inserter(test), to_remove),
            to_remove);

        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
            test.begin(), test.end() );
            
        std::vector<value_type> test2;
        test_append(
            boost::remove_copy(boost::make_iterator_range(c),
                               std::back_inserter(test2), to_remove),
            to_remove);
                               
        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                       test2.begin(), test2.end() );
    }

    template< class Container >
    void test_remove_copy_impl()
    {
        using namespace boost::assign;

        Container cont;
        test_remove_copy_impl(cont, 0);

        cont.clear();
        cont += 1;
        test_remove_copy_impl(cont, 0);
        test_remove_copy_impl(cont, 1);

        cont.clear();
        cont += 1,1,1,1,1;
        test_remove_copy_impl(cont, 0);
        test_remove_copy_impl(cont, 1);

        cont.clear();
        cont += 1,2,3,4,5,6,7,8,9;
        test_remove_copy_impl(cont, 1);
        test_remove_copy_impl(cont, 9);
        test_remove_copy_impl(cont, 4);
    }

    void test_remove_copy()
    {
        test_remove_copy_impl< std::vector<int> >();
        test_remove_copy_impl< std::list<int> >();
        test_remove_copy_impl< std::deque<int> >();
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.remove_copy" );

    test->add( BOOST_TEST_CASE( &test_remove_copy ) );

    return test;
}

