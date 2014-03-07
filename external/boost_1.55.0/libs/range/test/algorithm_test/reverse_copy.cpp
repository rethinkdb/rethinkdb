//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/algorithm/reverse_copy.hpp>

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
    void test_append(OutputIterator out, Value value)
    {
        *out++ = value;
    }

    template<class Container>
    void test_reverse_copy_impl(Container& cont)
    {
        typedef BOOST_DEDUCED_TYPENAME boost::range_value<Container>::type value_type;
        std::vector<value_type> reference;
        std::vector<value_type> test;

        test_append(
            std::reverse_copy(cont.begin(), cont.end(), std::back_inserter(reference)),
            value_type()
            );

        test_append(
            boost::reverse_copy(cont, std::back_inserter(test)),
            value_type()
            );

        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                       test.begin(), test.end() );
            
        test.clear();
        
        test_append(
            boost::reverse_copy(boost::make_iterator_range(cont),
                                std::back_inserter(test)),
            value_type()
            );
            
        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                       test.begin(), test.end() );
    }

    template<class Container>
    void test_reverse_copy_impl()
    {
        using namespace boost::assign;

        Container cont;
        test_reverse_copy_impl(cont);

        cont.clear();
        cont += 1;
        test_reverse_copy_impl(cont);

        cont.clear();
        cont += 1,2,3,4,5,6,7,8,9;
        test_reverse_copy_impl(cont);
    }

    void test_reverse_copy()
    {
        test_reverse_copy_impl< std::vector<int> >();
        test_reverse_copy_impl< std::list<int> >();
        test_reverse_copy_impl< std::deque<int> >();
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.reverse_copy" );

    test->add( BOOST_TEST_CASE( &::test_reverse_copy ) );

    return test;
}
