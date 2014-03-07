#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test )
{
    int i = 1;
    int j = 4;
    BOOST_CHECK_GE( i, j );
}

//____________________________________________________________________________//
