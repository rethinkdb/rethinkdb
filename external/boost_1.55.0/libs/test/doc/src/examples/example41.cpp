#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test )
{
    double v = -1.23456e-3;

    BOOST_CHECK_SMALL( v, 0.000001 );
}

//____________________________________________________________________________//
