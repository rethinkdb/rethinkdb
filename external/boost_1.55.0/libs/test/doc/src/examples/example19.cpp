#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>

struct F {
    F() : i( 0 ) { BOOST_TEST_MESSAGE( "setup fixture" ); }
    ~F()         { BOOST_TEST_MESSAGE( "teardown fixture" ); }

    int i;
};

//____________________________________________________________________________//

BOOST_FIXTURE_TEST_SUITE( s, F )

BOOST_AUTO_TEST_CASE( test_case1 )
{
    BOOST_CHECK( i == 1 );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_case2 )
{
    BOOST_CHECK_EQUAL( i, 0 );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()

