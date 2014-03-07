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
#include <boost/range/adaptor/indirected.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <algorithm>
#include <list>
#include <set>
#include <vector>

namespace boost
{
    namespace
    {

        template< class Container >
        void indirected_test_impl( Container& c )
        {
            using namespace boost::adaptors;

            // This is my preferred syntax using the | operator.
            std::vector< int > test_result1;
            boost::push_back(test_result1, c | indirected);

            // This is an alternative syntax preferred by some.
            std::vector< int > test_result2;
            boost::push_back(test_result2, adaptors::indirect(c));

            // Calculate the reference result.
            std::vector< int > reference_result;
            typedef BOOST_DEDUCED_TYPENAME Container::const_iterator iter_t;
            for (iter_t it = c.begin(); it != c.end(); ++it)
            {
                reference_result.push_back(**it);
            }

            BOOST_CHECK_EQUAL_COLLECTIONS( reference_result.begin(),
                                           reference_result.end(),
                                           test_result1.begin(),
                                           test_result1.end() );

            BOOST_CHECK_EQUAL_COLLECTIONS( reference_result.begin(),
                                           reference_result.end(),
                                           test_result2.begin(),
                                           test_result2.end() );
        }

        template< class Container >
        void indirected_test_impl()
        {
            using namespace boost::assign;

            Container c;

            indirected_test_impl(c);

            c += boost::shared_ptr<int>(new int(1));
            indirected_test_impl(c);

            std::vector<int> v;
            v += 1,1,2,2,2,3,4,4,4,4,5,6,7,8,9,9;
            for (std::vector<int>::const_iterator it = v.begin(); it != v.end(); ++it)
            {
                c += boost::shared_ptr<int>(new int(*it));
            }
            indirected_test_impl(c);
        }

        void indirected_test()
        {
            indirected_test_impl< std::vector< boost::shared_ptr< int > > >();
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.adaptor.indirected" );

    test->add( BOOST_TEST_CASE( &boost::indirected_test ) );

    return test;
}
