//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_TEST_TEST_DRIVER_RANGE_RETURN_TEST_DRIVER_HPP_INCLUDED
#define BOOST_RANGE_TEST_TEST_DRIVER_RANGE_RETURN_TEST_DRIVER_HPP_INCLUDED

#include <boost/assert.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

namespace boost
{
    namespace range_test
    {
        // check the results of an algorithm that returns
        // a range_return.
        //
        // This version is the general version. It should never be called.
        // All calls should invoke specialized implementations.
        template< range_return_value return_type >
        struct check_results
        {
            template< class Container, class Iterator >
            static void test(
                Container&          test,
                Container&          reference,
                Iterator            test_it,
                Iterator            reference_it
                )
            {
                BOOST_ASSERT( false );
            }
        };

        // check the results of an algorithm that returns
        // a 'found' iterator
        template< >
        struct check_results<return_found>
        {
            template< class Container, class Iterator >
            static void test(
                Container&          test,
                Container&          reference,
                Iterator            test_it,
                Iterator            reference_it
                )
            {
                BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                    test.begin(), test.end() );

                BOOST_CHECK_EQUAL( std::distance(test.begin(), test_it),
                   std::distance(reference.begin(), reference_it) );
            }
        };

        // check the results of an algorithm that returns
        // a 'next(found)' iterator
        template< >
        struct check_results<return_next>
        {
            template< class Container, class Iterator >
            static void test(
                Container&          test,
                Container&          reference,
                Iterator            test_it,
                Iterator            reference_it
                )
            {
                BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                    test.begin(), test.end() );

                if (reference_it == reference.end())
                {
                    BOOST_CHECK( test_it == test.end() );
                }
                else
                {
                    BOOST_CHECK_EQUAL(
                        std::distance(test.begin(), test_it),
                        std::distance(reference.begin(), reference_it) + 1);
                }
            }
        };

        // check the results of an algorithm that returns
        // a 'prior(found)' iterator
        template< >
        struct check_results<return_prior>
        {
            template< class Container, class Iterator >
            static void test(
                Container&          test,
                Container&          reference,
                Iterator            test_it,
                Iterator            reference_it
                )
            {
                BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                    test.begin(), test.end() );

                if (reference_it == reference.begin())
                {
                    BOOST_CHECK( test_it == test.begin() );
                }
                else
                {
                    BOOST_CHECK_EQUAL(
                        std::distance(test.begin(), test_it) + 1,
                        std::distance(reference.begin(), reference_it));
                }
            }
        };

        // check the results of an algorithm that returns
        // a '[begin, found)' range
        template< >
        struct check_results<return_begin_found>
        {
            template< class Container, class Iterator >
            static void test(
                Container&                  test,
                Container&                  reference,
                iterator_range<Iterator>    test_rng,
                Iterator                    reference_it
                )
            {
                BOOST_CHECK_EQUAL_COLLECTIONS(
                    reference.begin(), reference.end(),
                    test.begin(), test.end()
                    );

                BOOST_CHECK( test_rng.begin() == test.begin() );

                BOOST_CHECK_EQUAL_COLLECTIONS(
                    reference.begin(), reference_it,
                    boost::begin(test_rng), boost::end(test_rng)
                    );
            }
        };

        // check the results of an algorithm that returns
        // a '[begin, next(found))' range
        template< >
        struct check_results<return_begin_next>
        {
            template< class Container, class Iterator >
            static void test(
                Container&                  test,
                Container&                  reference,
                iterator_range<Iterator>    test_rng,
                Iterator                    reference_it
                )
            {
                BOOST_CHECK_EQUAL_COLLECTIONS(
                    reference.begin(), reference.end(),
                    test.begin(), test.end()
                    );

                BOOST_CHECK( test_rng.begin() == test.begin() );

                if (reference_it == reference.end())
                {
                    BOOST_CHECK( test_rng.end() == test.end() );
                }
                else
                {
                    BOOST_CHECK_EQUAL_COLLECTIONS(
                        reference.begin(), boost::next(reference_it),
                        test_rng.begin(), test_rng.end());
                }
            }
        };

        // check the results of an algorithm that returns
        // a '[begin, prior(found))' range
        template< >
        struct check_results<return_begin_prior>
        {
            template< class Container, class Iterator >
            static void test(
                Container&                  test,
                Container&                  reference,
                iterator_range<Iterator>    test_rng,
                Iterator                    reference_it
                )
            {
                BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                    test.begin(), test.end() );

                BOOST_CHECK( test_rng.begin() == test.begin() );

                if (reference_it == reference.begin())
                {
                    BOOST_CHECK( boost::end(test_rng) == test.begin() );
                }
                else
                {
                    BOOST_CHECK_EQUAL( std::distance(boost::begin(test_rng), boost::end(test_rng)) + 1,
                                       std::distance(reference.begin(), reference_it) );
                }
            }
        };

        // check the results of an algorithm that returns
        // a '[found, end)' range
        template< >
        struct check_results<return_found_end>
        {
            template< class Container, class Iterator >
            static void test(
                Container&                  test,
                Container&                  reference,
                iterator_range<Iterator>    test_rng,
                Iterator                    reference_it
                )
            {
                BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                    test.begin(), test.end() );

                BOOST_CHECK_EQUAL(
                    std::distance(test.begin(), boost::begin(test_rng)),
                    std::distance(reference.begin(), reference_it));

                BOOST_CHECK( boost::end(test_rng) == test.end() );
            }
        };

        // check the results of an algorithm that returns
        // a '[next(found), end)' range
        template< >
        struct check_results<return_next_end>
        {
            template< class Container, class Iterator >
            static void test(
                Container&                  test,
                Container&                  reference,
                iterator_range<Iterator>    test_rng,
                Iterator                    reference_it
                )
            {
                BOOST_CHECK_EQUAL_COLLECTIONS(
                    reference.begin(), reference.end(),
                    test.begin(), test.end()
                    );

                BOOST_CHECK( test_rng.end() == test.end() );

                if (reference_it == reference.end())
                {
                    BOOST_CHECK( test_rng.begin() == test.end() );
                }
                else
                {
                    BOOST_CHECK_EQUAL_COLLECTIONS(
                        boost::next(reference_it), reference.end(),
                        test_rng.begin(), test_rng.end()
                        );
                }
            }
        };

        // check the results of an algorithm that returns
        // a 'prior(found), end)' range
        template< >
        struct check_results<return_prior_end>
        {
            template< class Container, class Iterator >
            static void test(
                Container&                  test,
                Container&                  reference,
                iterator_range<Iterator>    test_rng,
                Iterator                    reference_it
                )
            {
                BOOST_CHECK_EQUAL_COLLECTIONS(
                    reference.begin(), reference.end(),
                    test.begin(), test.end()
                    );

                BOOST_CHECK( test_rng.end() == test.end() );

                if (reference_it == reference.begin())
                {
                    BOOST_CHECK( test_rng.begin() == test.begin() );
                }
                else
                {
                    BOOST_CHECK_EQUAL_COLLECTIONS(
                        boost::prior(reference_it), reference.end(),
                        test_rng.begin(), test_rng.end()
                        );
                }
            }
        };

        // check the results of an algorithm that returns
        // a '[begin, end)' range
        template< >
        struct check_results<return_begin_end>
        {
            template< class Container, class Iterator >
            static void test(
                Container&                  test,
                Container&                  reference,
                iterator_range<Iterator>    test_rng,
                Iterator                    reference_it
                )
            {
                BOOST_CHECK_EQUAL_COLLECTIONS(
                    reference.begin(), reference.end(),
                    test.begin(), test.end()
                    );

                BOOST_CHECK( test_rng.begin() == test.begin() );
                BOOST_CHECK( test_rng.end() == test.end() );
            }
        };

        // A test driver to exercise a test through all of the range_return
        // combinations.
        //
        // The test driver also contains the code required to check the
        // return value correctness.
        //
        // The TestPolicy needs to implement two functions:
        //
        // - perform the boost range version of the algorithm that returns
        //   a range_return<Container,return_type>::type
        // template<range_return_value return_type, class Container>
        // BOOST_DEDUCED_TYPENAME range_return<Container,return_type>::type
        // test(Container& cont);
        //
        // - perform the reference std version of the algorithm that
        //   returns the standard iterator result
        // template<class Container>
        // BOOST_DEDUCED_TYPENAME range_iterator<Container>::type
        // reference(Container& cont);
        class range_return_test_driver
        {
        public:
            template< class Container,
                      class TestPolicy >
            void operator()(Container& cont, TestPolicy policy)
            {
                test_range_iter               (cont, policy);
                test_range<return_found,Container,TestPolicy>      ()(cont, policy);
                test_range<return_next,Container,TestPolicy>       ()(cont, policy);
                test_range<return_prior,Container,TestPolicy>      ()(cont, policy);
                test_range<return_begin_found,Container,TestPolicy>()(cont, policy);
                test_range<return_begin_next,Container,TestPolicy> ()(cont, policy);
                test_range<return_begin_prior,Container,TestPolicy>()(cont, policy);
                test_range<return_found_end,Container,TestPolicy>  ()(cont, policy);
                test_range<return_next_end,Container,TestPolicy>   ()(cont, policy);
                test_range<return_prior_end,Container,TestPolicy>  ()(cont, policy);
                test_range<return_begin_end,Container,TestPolicy>  ()(cont, policy);
            }

        private:
            template< class Container, class TestPolicy >
            void test_range_iter(
                Container&          cont,
                TestPolicy          policy
                )
            {
                typedef BOOST_DEDUCED_TYPENAME range_iterator<Container>::type iterator_t;

                Container reference(cont);
                Container test(cont);

                iterator_t range_result = policy.test_iter(test);
                iterator_t reference_it = policy.reference(reference);

                check_results<return_found>::test(test, reference,
                                                  range_result, reference_it);
            }

            template< range_return_value result_type, class Container, class TestPolicy >
            struct test_range
            {
                void operator()(Container& cont, TestPolicy policy)
                {
                    typedef BOOST_DEDUCED_TYPENAME range_iterator<Container>::type iterator_t;
                    typedef BOOST_DEDUCED_TYPENAME range_return<Container, result_type>::type range_return_t;
                    typedef BOOST_DEDUCED_TYPENAME TestPolicy::template test_range<result_type> test_range_t;

                    Container reference(cont);
                    Container test_cont(cont);

                    test_range_t test_range_fn;
                    range_return_t range_result = test_range_fn(policy, test_cont);
                    iterator_t reference_it = policy.reference(reference);

                    check_results<result_type>::test(test_cont, reference,
                                                     range_result, reference_it);
                }
            };
        };
    }
}

#endif // include guard
