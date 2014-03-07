#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test )
{
    BOOST_CHECK_BITWISE_EQUAL( (char)0x26, 0x04 );
}

//____________________________________________________________________________//
