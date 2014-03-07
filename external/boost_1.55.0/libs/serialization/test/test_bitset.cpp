//  (C) Copyright 2009 Brian Ravnsgaard and Kenneth Riddile
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version including documentation.

// Test that serialization of std::bitset works.
// Should pass compilation and execution
// 16.09.2004, updated 04.03.2009

#include <cstddef> // NULL
#include <cstdio> // remove
#include <fstream>

#include <boost/config.hpp>

#if defined( BOOST_NO_STDC_NAMESPACE )
namespace std
{ 
    using ::remove;
}
#endif

#include "test_tools.hpp"

#include <boost/serialization/bitset.hpp>
#include <boost/serialization/nvp.hpp>

int test_main( int /* argc */, char* /* argv */[] )
{
    const char* testfile = boost::archive::tmpnam( NULL );
    BOOST_REQUIRE( NULL != testfile );

    std::bitset<8> bitsetA;
    bitsetA.set( 0, false );
    bitsetA.set( 1, true  );
    bitsetA.set( 2, false );
    bitsetA.set( 3, true  );
    bitsetA.set( 4, false );
    bitsetA.set( 5, false );
    bitsetA.set( 6, true  );
    bitsetA.set( 7, true  );
    
    {
        test_ostream os( testfile, TEST_STREAM_FLAGS );
        test_oarchive oa( os );
        oa << boost::serialization::make_nvp( "bitset", bitsetA );
    }
    
    std::bitset<8> bitsetB;
    {
        test_istream is( testfile, TEST_STREAM_FLAGS );
        test_iarchive ia( is );
        ia >> boost::serialization::make_nvp( "bitset", bitsetB );
    }
    
    BOOST_CHECK( bitsetA == bitsetB );
    
    std::remove( testfile );
    return EXIT_SUCCESS;
}

// EOF
