#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test )
{
    int i=2;
    BOOST_WARN( sizeof(int) == sizeof(short) );
    BOOST_CHECK( i == 1 );
    BOOST_REQUIRE( i > 5 );
    BOOST_CHECK( i == 6 ); // will never reach this check
}

//____________________________________________________________________________//
