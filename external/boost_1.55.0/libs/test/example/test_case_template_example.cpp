//  (C) Copyright Gennadiy Rozental 2001-2006.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
#ifdef BOOST_MSVC
# pragma warning(disable: C4345)
#endif

#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

// Boost.MPL
#include <boost/mpl/range_c.hpp>

BOOST_TEST_CASE_TEMPLATE_FUNCTION( free_test_function, Number )
{
    BOOST_CHECK_EQUAL( 2, (int const)Number::value );
}

test_suite*
init_unit_test_suite( int, char* [] ) {
    test_suite* test= BOOST_TEST_SUITE( "Test case template example" );

    typedef boost::mpl::range_c<int,0,10> numbers;

    test->add( BOOST_TEST_CASE_TEMPLATE( free_test_function, numbers ) );

    return test;
}

// EOF
