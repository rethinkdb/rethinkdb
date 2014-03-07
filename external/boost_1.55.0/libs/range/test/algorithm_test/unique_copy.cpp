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
#include <boost/range/algorithm/unique_copy.hpp>

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
    template<class OutputIterator, class Value>
    void test_append(OutputIterator target, Value value)
    {
        *target++ = value;
    }

    template<class Container>
    void test_unique_copy_impl(Container& c)
    {
        typedef BOOST_DEDUCED_TYPENAME boost::range_value<Container>::type value_type;
        std::vector<value_type> reference;
        std::vector<value_type> test;

        test_append(
            std::unique_copy(c.begin(), c.end(), std::back_inserter(reference)),
            value_type()
            );

        test_append(
            boost::unique_copy(c, std::back_inserter(test)),
            value_type()
            );

        BOOST_CHECK_EQUAL_COLLECTIONS(reference.begin(), reference.end(),
                                      test.begin(), test.end());
                                      
        test.clear();
        
        test_append(
            boost::unique_copy(boost::make_iterator_range(c),
                               std::back_inserter(test)),
            value_type()
            );
            
        BOOST_CHECK_EQUAL_COLLECTIONS(reference.begin(), reference.end(),
                                      test.begin(), test.end());
    }

    template<class Container, class Pred>
    void test_unique_copy_impl(Container& c, Pred pred)
    {
        typedef BOOST_DEDUCED_TYPENAME boost::range_value<Container>::type value_type;
        std::vector<value_type> reference;
        std::vector<value_type> test;

        test_append(
            std::unique_copy(c.begin(), c.end(), std::back_inserter(reference), pred),
            value_type()
            );

        test_append(
            boost::unique_copy(c, std::back_inserter(test), pred),
            value_type()
            );

        BOOST_CHECK_EQUAL_COLLECTIONS(reference.begin(), reference.end(),
                                      test.begin(), test.end());
                                      
        test.clear();
        
        test_append(
            boost::unique_copy(boost::make_iterator_range(c),
                               std::back_inserter(test), pred),
            value_type()
            );
            
        BOOST_CHECK_EQUAL_COLLECTIONS(reference.begin(), reference.end(),
                                      test.begin(), test.end());
    }

    template<class Container, class Pred>
    void test_unique_copy_driver(Pred pred)
    {
        using namespace boost::assign;

        typedef BOOST_DEDUCED_TYPENAME Container::value_type value_t;

        Container cont;

        test_unique_copy_impl(cont);
        test_unique_copy_impl(cont, pred);

        cont.clear();
        cont += 1;

        std::vector<value_t> temp(cont.begin(), cont.end());
        std::sort(temp.begin(), temp.end());
        cont.assign(temp.begin(), temp.end());
        test_unique_copy_impl(cont);

        std::sort(temp.begin(), temp.end(), pred);
        cont.assign(temp.begin(), temp.end());
        test_unique_copy_impl(cont, pred);

        cont.clear();
        cont += 1,2,2,2,2,3,4,5,6,7,8,9;

        temp.assign(cont.begin(), cont.end());
        std::sort(temp.begin(), temp.end());
        cont.assign(temp.begin(), temp.end());
        test_unique_copy_impl(cont);

        std::sort(temp.begin(), temp.end(), pred);
        cont.assign(temp.begin(), temp.end());
        test_unique_copy_impl(cont, pred);
    }

    template<class Container>
    void test_unique_copy_impl()
    {
        test_unique_copy_driver<Container>(std::less<int>());
        test_unique_copy_driver<Container>(std::greater<int>());
    }

    void test_unique_copy()
    {
        test_unique_copy_impl< std::vector<int> >();
        test_unique_copy_impl< std::list<int> >();
        test_unique_copy_impl< std::deque<int> >();
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.unique_copy" );

    test->add( BOOST_TEST_CASE( &test_unique_copy ) );

    return test;
}
