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
#include <boost/range/adaptor/map.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/array.hpp>
#include <boost/assign.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <algorithm>
#include <map>
#include <vector>

namespace boost
{
    namespace
    {
        template< class Container >
        void map_test_keys( Container& c )
        {
            using namespace boost::adaptors;

            std::vector<int> keys;
            boost::push_back(keys, c | map_keys);

            std::vector<int> keys2;
            boost::push_back(keys2, adaptors::keys(c));

            std::vector<int> reference_keys;
            typedef BOOST_DEDUCED_TYPENAME Container::const_iterator iter_t;
            for (iter_t it = c.begin(); it != c.end(); ++it)
            {
                reference_keys.push_back(it->first);
            }

            BOOST_CHECK_EQUAL_COLLECTIONS( reference_keys.begin(),
                                           reference_keys.end(),
                                           keys.begin(),
                                           keys.end() );

            BOOST_CHECK_EQUAL_COLLECTIONS( reference_keys.begin(),
                                           reference_keys.end(),
                                           keys2.begin(),
                                           keys2.end() );
        }

        template< class Container >
        void map_test_values( Container& c )
        {
            using namespace boost::adaptors;

            std::vector<int> values;
            boost::push_back(values, c | map_values);

            std::vector<int> values2;
            boost::push_back(values2, adaptors::values(c));

            std::vector<int> reference_values;
            typedef BOOST_DEDUCED_TYPENAME Container::const_iterator iter_t;
            for (iter_t it = c.begin(); it != c.end(); ++it)
            {
                reference_values.push_back(it->second);
            }

            BOOST_CHECK_EQUAL_COLLECTIONS( reference_values.begin(),
                                           reference_values.end(),
                                           values.begin(),
                                           values.end() );

            BOOST_CHECK_EQUAL_COLLECTIONS( reference_values.begin(),
                                           reference_values.end(),
                                           values2.begin(),
                                           values2.end() );
        }

        template< class Container >
        void map_test_impl()
        {
            using namespace boost::assign;

            Container c;

            // Test empty
            map_test_keys(c);
            map_test_values(c);

            // Test one element
            c.insert(std::make_pair(1,2));
            map_test_keys(c);
            map_test_values(c);

            // Test many elements
            for (int x = 2; x < 10; ++x)
            {
                c.insert(std::make_pair(x, x * 2));
            }
            map_test_keys(c);
            map_test_values(c);
        }

        void map_test()
        {
            map_test_impl< std::map<int,int> >();
        }

        void test_trac_item_4388()
        {
            typedef std::pair<int,char> pair_t;
            const boost::array<pair_t,3> ar = {{
                pair_t(3, 'a'),
                pair_t(1, 'b'),
                pair_t(4, 'c')
            }};

            const boost::array<int, 3> expected_keys = {{ 3, 1, 4 }};
            const boost::array<char, 3> expected_values = {{ 'a', 'b', 'c' }};

            {
                std::vector<int> test;
                boost::push_back(test, ar | boost::adaptors::map_keys);
                BOOST_CHECK_EQUAL_COLLECTIONS(
                    expected_keys.begin(), expected_keys.end(),
                    test.begin(), test.end()
                );
            }

            {
                std::vector<char> test;
                boost::push_back(test, ar | boost::adaptors::map_values);
                BOOST_CHECK_EQUAL_COLLECTIONS(
                    expected_values.begin(), expected_values.end(),
                    test.begin(), test.end()
                );
            }

            {
                std::vector<char> test;
                boost::array<std::pair<int, char>, 3> src(ar);
                boost::push_back(test, src | boost::adaptors::map_values);
                BOOST_CHECK_EQUAL_COLLECTIONS(
                    expected_values.begin(), expected_values.end(),
                    test.begin(), test.end()
                );
            }
        }

    }
}


boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.adaptor.map" );

    test->add( BOOST_TEST_CASE( &boost::map_test ) );
    test->add( BOOST_TEST_CASE( &boost::test_trac_item_4388 ) );

    return test;
}
