#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>

struct F {
    F() : i( 0 ) { BOOST_TEST_MESSAGE( "setup fixture" ); }
    ~F()         { BOOST_TEST_MESSAGE( "teardown fixture" ); }

    int i;
};

BOOST_FIXTURE_TEST_CASE( test_case1, F )
{
    BOOST_CHECK( i == 1 );
    ++i;
}

BOOST_FIXTURE_TEST_CASE( test_case2, F )
{
    BOOST_CHECK_EQUAL( i, 1 );
}

BOOST_AUTO_TEST_CASE( test_case3 )
{
    BOOST_CHECK( true );
}
