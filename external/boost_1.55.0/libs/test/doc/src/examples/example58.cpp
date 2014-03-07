#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test )
{
    int i = 2;
    int j = 14;
    BOOST_CHECK_GT( i, j );
}

//____________________________________________________________________________//
