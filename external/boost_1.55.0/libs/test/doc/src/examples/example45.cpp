#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>

//____________________________________________________________________________//

class my_exception{};

BOOST_AUTO_TEST_CASE( test )
{
    int i =  0;
    BOOST_CHECK_THROW( i++, my_exception );
}

//____________________________________________________________________________//
