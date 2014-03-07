#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test )
{
    int col1 [] = { 1, 2, 3, 4, 5, 6, 7 };
    int col2 [] = { 1, 2, 4, 4, 5, 7, 7 };

    BOOST_CHECK_EQUAL_COLLECTIONS( col1, col1+7, col2, col2+7 );
}

//____________________________________________________________________________//
