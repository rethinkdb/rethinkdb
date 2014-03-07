/*
 [auto_generated]
 libs/numeric/odeint/test/same_size.cpp

 [begin_description]
 tba.
 [end_description]

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/config.hpp>
#ifdef BOOST_MSVC
    #pragma warning(disable:4996)
#endif

#define BOOST_TEST_MODULE odeint_dummy

#include <boost/test/unit_test.hpp>

#include <boost/numeric/odeint/util/same_size.hpp>

using namespace boost::unit_test;
using namespace boost::numeric::odeint;


BOOST_AUTO_TEST_SUITE( same_size_test )

BOOST_AUTO_TEST_CASE( test_vector_true )
{
    std::vector< double > v1( 10 ) , v2( 10 );
    BOOST_CHECK_EQUAL( true , same_size( v1 , v2 ) );
}


BOOST_AUTO_TEST_CASE( test_vector_false )
{
    std::vector< double > v1( 10 ) , v2( 20 );
    BOOST_CHECK_EQUAL( false , same_size( v1 , v2 ) );
}

BOOST_AUTO_TEST_CASE( test_fusion_true )
{
    boost::fusion::vector< double , std::vector< double > > v1 , v2;
    boost::fusion::at_c< 1 >( v1 ).resize( 10 );
    boost::fusion::at_c< 1 >( v2 ).resize( 10 );
    BOOST_CHECK_EQUAL( true , same_size( v1 , v2 ) );
}

BOOST_AUTO_TEST_CASE( test_fusion_false )
{
    boost::fusion::vector< double , std::vector< double > > v1 , v2;
    boost::fusion::at_c< 1 >( v1 ).resize( 10 );
    boost::fusion::at_c< 1 >( v2 ).resize( 20 );
    BOOST_CHECK_EQUAL( false , same_size( v1 , v2 ) );
}



BOOST_AUTO_TEST_SUITE_END()
