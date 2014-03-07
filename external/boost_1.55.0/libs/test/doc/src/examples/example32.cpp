#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
#include <utility>

//____________________________________________________________________________//

typedef std::pair<int,float> pair_type;

BOOST_TEST_DONT_PRINT_LOG_VALUE( pair_type );

BOOST_AUTO_TEST_CASE( test_list )
{
    pair_type p1( 2, 5.5 );
    pair_type p2( 2, 5.501 );

    BOOST_CHECK_EQUAL( p1, p2 );
}

//____________________________________________________________________________//
