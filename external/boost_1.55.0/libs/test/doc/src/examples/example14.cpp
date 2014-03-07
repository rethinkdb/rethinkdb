#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE my master test suite name
#include <boost/test/included/unit_test.hpp>

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( free_test_function )
{
    BOOST_CHECK( true /* test assertion */ );
}

//____________________________________________________________________________//

