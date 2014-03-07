//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/algorithm/random_shuffle.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include "../test_function/counted_function.hpp"
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
        template<class Int>
        class counted_generator
            : private range_test_function::counted_function
        {
        public:
            typedef Int result_type;
            typedef Int argument_type;

            using range_test_function::counted_function::invocation_count;

            result_type operator()(argument_type modulo_value)
            {
                invoked();
                return static_cast<result_type>(std::rand() % modulo_value);
            }
        };

        template<class Container>
        bool test_shuffle_result(
            const Container& old_cont,
            const Container& new_cont
            )
        {
            typedef BOOST_DEDUCED_TYPENAME range_iterator<const Container>::type iterator_t;

            // The size must remain equal
            BOOST_CHECK_EQUAL( old_cont.size(), new_cont.size() );
            if (old_cont.size() != new_cont.size())
                return false;

            if (new_cont.size() < 2)
            {
                BOOST_CHECK_EQUAL_COLLECTIONS(
                    old_cont.begin(), old_cont.end(),
                    new_cont.begin(), new_cont.end()
                    );

                return std::equal(old_cont.begin(), old_cont.end(),
                                  new_cont.begin());
            }

            // Elements must only be moved around. This is tested by
            // ensuring the count of each element remains the
            // same.
            bool failed = false;
            iterator_t last = old_cont.end();
            for (iterator_t it = old_cont.begin(); !failed && (it != last); ++it)
            {
                const std::size_t old_count
                    = std::count(old_cont.begin(), old_cont.end(), *it);

                const std::size_t new_count
                    = std::count(new_cont.begin(), new_cont.end(), *it);

                BOOST_CHECK_EQUAL( old_count, new_count );

                failed = (old_count != new_count);
            }

            return !failed;
        }

        template<class Container>
        void test_random_shuffle_nogen_impl(Container& cont)
        {
            typedef BOOST_DEDUCED_TYPENAME range_iterator<Container>::type iterator_t;

            const int MAX_RETRIES = 10000;

            bool shuffled = false;
            for (int attempt = 0; !shuffled && (attempt < MAX_RETRIES); ++attempt)
            {
                Container test(cont);
                boost::random_shuffle(test);
                bool ok = test_shuffle_result(cont, test);
                if (!ok)
                    break;

                // Since the counts are equal, then if they are 
                // not equal the contents must have been shuffled
                if (cont.size() == test.size()
                &&  !std::equal(cont.begin(), cont.end(), test.begin()))
                {
                    shuffled = true;
                }
                
                // Verify that the shuffle can be performed on a
                // temporary range
                Container test2(cont);
                boost::random_shuffle(boost::make_iterator_range(test2));
                ok = test_shuffle_result(cont, test2);
                if (!ok)
                    break;
            }
        }

        template<class RandomGenerator, class Container>
        void test_random_shuffle_gen_impl(Container& cont)
        {
            RandomGenerator gen;
            Container old_cont(cont);
            boost::random_shuffle(cont, gen);
            test_shuffle_result(cont, old_cont);
            if (cont.size() > 2)
            {
                BOOST_CHECK( gen.invocation_count() > 0 );
            }
            
            // Test that random shuffle works when 
            // passed a temporary range
            RandomGenerator gen2;
            Container cont2(old_cont);
            boost::random_shuffle(boost::make_iterator_range(cont2), gen2);
            test_shuffle_result(cont2, old_cont);
            if (cont2.size() > 2)
            {
                BOOST_CHECK( gen2.invocation_count() > 0 );
            }
        }

        template<class Container>
        void test_random_shuffle_impl(Container& cont)
        {
            Container old_cont(cont);
            boost::random_shuffle(cont);
            test_shuffle_result(cont, old_cont);
        }

        template<class Container>
        void test_random_shuffle_impl()
        {
            using namespace boost::assign;

            typedef counted_generator<
                BOOST_DEDUCED_TYPENAME range_difference<Container>::type > generator_t;

            Container cont;
            test_random_shuffle_nogen_impl(cont);
            test_random_shuffle_gen_impl<generator_t>(cont);

            cont.clear();
            cont += 1;
            test_random_shuffle_nogen_impl(cont);
            test_random_shuffle_gen_impl<generator_t>(cont);

            cont.clear();
            cont += 1,2,3,4,5,6,7,8,9;
            test_random_shuffle_nogen_impl(cont);
            test_random_shuffle_gen_impl<generator_t>(cont);
        }

        void test_random_shuffle()
        {
            test_random_shuffle_impl< std::vector<int> >();
            test_random_shuffle_impl< std::deque<int> >();
        }
    }
}


boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.random_shuffle" );

    test->add( BOOST_TEST_CASE( &boost::test_random_shuffle ) );

    return test;
}
