#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>

//____________________________________________________________________________//

#include <cmath>

BOOST_AUTO_TEST_CASE( test )
{
    double res = std::sin( 45. );

    BOOST_WARN_MESSAGE( res > 1, "sin(45){" << res << "} is <= 1. Hmm.. Strange. " );
}

//____________________________________________________________________________//
