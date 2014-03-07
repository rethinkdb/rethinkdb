#include <boost/test/minimal.hpp>

//____________________________________________________________________________//

int add( int i, int j ) { return i+j; }

//____________________________________________________________________________//

int test_main( int, char *[] )             // note the name!
{
    // six ways to detect and report the same error:
    BOOST_CHECK( add( 2,2 ) == 4 );        // #1 continues on error
    BOOST_REQUIRE( add( 2,2 ) == 4 );      // #2 throws on error
    if( add( 2,2 ) != 4 )
        BOOST_ERROR( "Ouch..." );          // #3 continues on error
    if( add( 2,2 ) != 4 )
        BOOST_FAIL( "Ouch..." );           // #4 throws on error
    if( add( 2,2 ) != 4 ) throw "Oops..."; // #5 throws on error

    return add( 2, 2 ) == 4 ? 0 : 1;       // #6 returns error code
}

//____________________________________________________________________________//
