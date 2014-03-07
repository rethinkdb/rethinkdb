#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test )
{
    double v1 = 1.23456e28;
    double v2 = 1.23457e28;

    BOOST_REQUIRE_CLOSE( v1, v2, 0.001 );
    // Absolute value of difference between these two values is 1e+23.
    // But we are interested only that it does not exeed 0.001% of a values compared
    // And this test will pass.
}

//____________________________________________________________________________//
