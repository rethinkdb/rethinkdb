#include <boost/test/included/unit_test.hpp>
#include <boost/mpl/list.hpp>
using namespace boost::unit_test;

//____________________________________________________________________________//

BOOST_TEST_CASE_TEMPLATE_FUNCTION( my_test, T )
{
    BOOST_CHECK_EQUAL( sizeof(T), 4 );
}

//____________________________________________________________________________//

test_suite*
init_unit_test_suite( int argc, char* argv[] )
{
    typedef boost::mpl::list<int,long,unsigned char> test_types;

    framework::master_test_suite().
        add( BOOST_TEST_CASE_TEMPLATE( my_test, test_types ) );

    return 0;
}

//____________________________________________________________________________//
