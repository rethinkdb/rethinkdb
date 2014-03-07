#ifndef BOOST_TEST_DYN_LINK
#define BOOST_TEST_DYN_LINK
#endif
#include <boost/test/unit_test.hpp>
#include <boost/bind.hpp>
using namespace boost::unit_test;

//____________________________________________________________________________//

void free_test_function( int i, int j )
{
    BOOST_CHECK_EQUAL( i, j );
}

//____________________________________________________________________________//

bool
init_function()
{
    framework::master_test_suite().
        add( BOOST_TEST_CASE( boost::bind( &free_test_function, 1, 1 ) ) );
    framework::master_test_suite().
        add( BOOST_TEST_CASE( boost::bind( &free_test_function, 1, 2 ) ) );
    framework::master_test_suite().
        add( BOOST_TEST_CASE( boost::bind( &free_test_function, 2, 1 ) ) );

    // do your own initialization here
    // if it successful return true

    // But, you CAN'T use testing tools here

    return true;
}

//____________________________________________________________________________//

int
main( int argc, char* argv[] )
{
    return ::boost::unit_test::unit_test_main( &init_function, argc, argv );
}

//____________________________________________________________________________//
