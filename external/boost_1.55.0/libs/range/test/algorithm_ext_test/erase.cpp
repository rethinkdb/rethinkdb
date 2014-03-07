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
#include <boost/range/algorithm_ext/erase.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/range/iterator.hpp>
#include <algorithm>
#include <list>
#include <vector>

namespace
{
    template< class Container >
    void test_erase_impl()
    {
        Container source;
        for (int i = 0; i < 10; ++i)
            source.push_back(i);

        Container reference(source);
        Container test(source);

        typedef BOOST_DEDUCED_TYPENAME Container::iterator iterator_t;

        iterator_t first_ref = reference.begin();
        iterator_t last_ref = reference.end();

        boost::iterator_range< iterator_t > test_range(test.begin(), test.end());

        reference.erase(first_ref, last_ref);
        boost::erase(test, test_range);

        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
            test.begin(), test.end() );
    }

    void test_erase()
    {
        test_erase_impl<std::vector<int> >();
        test_erase_impl<std::list<int> >();
    }

    template< class Container >
    void test_remove_erase_impl()
    {
        Container source;
        for (int i = 0; i < 10; ++i)
            source.push_back(i);

        Container reference(source);
        Container test(source);

        boost::remove_erase(test, 5);

        reference.erase( std::find(reference.begin(), reference.end(), 5) );

        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
            test.begin(), test.end() );
    }

    void test_remove_erase()
    {
        test_remove_erase_impl<std::vector<int> >();
        test_remove_erase_impl<std::list<int> >();
    }

    struct is_even
    {
        typedef bool result_type;
        typedef int argument_type;
        bool operator()(int x) const { return x % 2 == 0; }
    };

    template< class Container >
    void test_remove_erase_if_impl()
    {
        Container source;
        for (int i = 0; i < 10; ++i)
            source.push_back(i);

        Container reference;
        typedef BOOST_DEDUCED_TYPENAME Container::const_iterator iterator_t;
        iterator_t last_source = source.end();
        is_even pred;
        for (iterator_t it_source = source.begin(); it_source != last_source; ++it_source)
        {
            if (!pred(*it_source))
                reference.push_back(*it_source);
        }

        Container test(source);
        boost::remove_erase_if(test, is_even());

        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
            test.begin(), test.end() );

    }

    void test_remove_erase_if()
    {
        test_remove_erase_if_impl<std::vector<int> >();
        test_remove_erase_if_impl<std::list<int> >();
    }

}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm_ext.erase" );

    test->add( BOOST_TEST_CASE( &test_erase ) );
    test->add( BOOST_TEST_CASE( &test_remove_erase ) );
    test->add( BOOST_TEST_CASE( &test_remove_erase_if ) );

    return test;
}
