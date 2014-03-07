#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test )
{
    int i = 7;
    int j = 7;
    BOOST_CHECK_LT( i, j );
}

//____________________________________________________________________________//
