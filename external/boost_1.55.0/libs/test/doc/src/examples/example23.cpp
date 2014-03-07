#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_case )
{
    int* p = 0;

    BOOST_TEST_PASSPOINT();
    ++p;

    BOOST_TEST_PASSPOINT();
    ++p;

    BOOST_TEST_PASSPOINT();
    int j = *p;
}

//____________________________________________________________________________//
