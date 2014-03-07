/*
 [auto_generated]
 libs/numeric/odeint/test/is_resizeable.cpp

 [begin_description]
 This file tests is_resizeable meta-function of odeint.
 [end_description]

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

// disable checked iterator warning for msvc
#include <boost/config.hpp>
#ifdef BOOST_MSVC
    #pragma warning(disable:4996)
#endif

#define BOOST_TEST_MODULE odeint_is_resizeable

#include <vector>
#include <cmath>

#include <boost/test/unit_test.hpp>

#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/time.hpp>
#include <boost/units/systems/si/velocity.hpp>
#include <boost/units/systems/si/acceleration.hpp>
#include <boost/units/systems/si/io.hpp>


using namespace boost::unit_test;
using namespace boost::numeric::odeint;

BOOST_AUTO_TEST_CASE( test_vector )
{
    BOOST_CHECK( is_resizeable< std::vector< int > >::value );
}

BOOST_AUTO_TEST_CASE( test_double )
{
    BOOST_CHECK( !( is_resizeable< double >::value ) );
}

BOOST_AUTO_TEST_CASE( test_fusion_vector_of_vector )
{
    typedef boost::fusion::vector< std::vector< double > , std::vector< double > > state_type;
    BOOST_CHECK( is_resizeable< state_type >::value );
}

BOOST_AUTO_TEST_CASE( test_fusion_vector_of_double )
{
    typedef boost::fusion::vector< double , double > state_type;
    BOOST_CHECK( !( is_resizeable< state_type >::value ) );
}

BOOST_AUTO_TEST_CASE( test_fusion_vector_mixed1 )
{
    typedef boost::fusion::vector< double , std::vector< double > > state_type;
    BOOST_CHECK( is_resizeable< state_type >::value);
}

BOOST_AUTO_TEST_CASE( test_fusion_vector_mixed2 )
{
    typedef boost::fusion::vector< std::vector< double > , double > state_type;
    BOOST_CHECK( is_resizeable< state_type >::value );
}

BOOST_AUTO_TEST_CASE( test_fusion_quantity_sequence )
{
    namespace units = boost::units;
    namespace si = boost::units::si;

    typedef double value_type;
    typedef units::quantity< si::time , value_type > time_type;
    typedef units::quantity< si::length , value_type > length_type;
    typedef units::quantity< si::velocity , value_type > velocity_type;
    typedef boost::fusion::vector< length_type , velocity_type > state_type;

    BOOST_CHECK( !( is_resizeable< state_type >::value ) );
}
