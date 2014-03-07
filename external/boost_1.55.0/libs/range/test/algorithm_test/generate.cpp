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
#include <boost/range/algorithm/generate.hpp>

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

namespace boost
{
    namespace
    {
        class generator_fn
        {
        public:
            typedef int result_type;

            generator_fn() : m_count(0) {}
            int operator()() { return ++m_count; }

        private:
            int m_count;
        };

        template< class Container >
        void test_generate_impl(Container& cont)
        {
            Container reference(cont);
            std::generate(reference.begin(), reference.end(), generator_fn());

            Container test(cont);
            boost::generate(test, generator_fn());

            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                test.begin(), test.end() );
                
            Container test2(cont);
            boost::generate(boost::make_iterator_range(test2), generator_fn());
            
            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           test2.begin(), test2.end() );
        }

        template< class Container >
        void test_generate_impl()
        {
            using namespace boost::assign;

            Container cont;
            test_generate_impl(cont);

            cont.clear();
            cont += 9;
            test_generate_impl(cont);

            cont.clear();
            cont += 9,8,7,6,5,4,3,2,1;
            test_generate_impl(cont);
        }

        void test_generate()
        {
            test_generate_impl< std::vector<int> >();
            test_generate_impl< std::list<int> >();
            test_generate_impl< std::deque<int> >();
        }
    }
}


boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.generate" );

    test->add( BOOST_TEST_CASE( &boost::test_generate ) );

    return test;
}
