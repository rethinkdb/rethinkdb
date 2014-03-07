#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES( my_test1, 1 )

BOOST_AUTO_TEST_CASE( my_test1 )
{
    BOOST_CHECK( 2 == 1 );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE( internal )

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES( my_test1, 2 )

BOOST_AUTO_TEST_CASE( my_test1 )
{
    BOOST_CHECK_EQUAL( sizeof(int), sizeof(char) );
    BOOST_CHECK_EQUAL( sizeof(int*), sizeof(char) );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
