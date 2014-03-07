#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test;

//____________________________________________________________________________//

bool moo( int arg1, int arg2, int mod ) { return ((arg1+arg2) % mod) == 0; }

BOOST_AUTO_TEST_CASE( test )
{
    int i = 17;
    int j = 15;
    unit_test_log.set_threshold_level( log_warnings );
    BOOST_WARN( moo( 12,i,j ) );
    BOOST_WARN_PREDICATE( moo, (12)(i)(j) );
}

//____________________________________________________________________________//
