#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>

//____________________________________________________________________________//

extern void foo( int i );

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_external_interface )
{
    for( int i = 3; i >=0; i-- ) {
        BOOST_TEST_CHECKPOINT( "Calling foo with i=" << i );
        foo( i );
    }
}

//____________________________________________________________________________//

void foo( int i )
{
    int j = 2/(i-1);
}

//____________________________________________________________________________//
