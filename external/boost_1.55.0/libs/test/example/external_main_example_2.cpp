#ifndef BOOST_TEST_DYN_LINK
#define BOOST_TEST_DYN_LINK
#endif
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test;

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE( test_suite_1 )

BOOST_AUTO_TEST_CASE( test_case_1 )
{
     BOOST_MESSAGE( "Testing is in progress" );

     BOOST_CHECK( false );
}

BOOST_AUTO_TEST_SUITE_END()

//____________________________________________________________________________//

bool
init_function()
{
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
