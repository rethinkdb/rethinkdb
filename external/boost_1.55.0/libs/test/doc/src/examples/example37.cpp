#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>

//____________________________________________________________________________//

struct my_exception {
    explicit my_exception( int ec = 0 ) : m_error_code( ec ) {}

    int m_error_code;
};

bool is_critical( my_exception const& ex ) { return ex.m_error_code < 0; }

BOOST_AUTO_TEST_CASE( test )
{
    BOOST_CHECK_EXCEPTION( throw my_exception( 1 ), my_exception, is_critical );
}

//____________________________________________________________________________//
