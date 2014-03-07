#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test )
{
    double v1 = 1.111e-10;
    double v2 = 1.112e-10;

    BOOST_CHECK_CLOSE_FRACTION( v1, v2, 0.0008999 );
}

//____________________________________________________________________________//
