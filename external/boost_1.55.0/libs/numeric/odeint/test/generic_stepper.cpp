/*
 [auto_generated]
 libs/numeric/odeint/test/generic_stepper.cpp

 [begin_description]
 This file tests the generic stepper.
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

#define BOOST_TEST_MODULE odeint_generic_stepper

#include <iostream>
#include <utility>

#include <boost/test/unit_test.hpp>

#include <boost/numeric/odeint/stepper/explicit_generic_rk.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta4.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta4_classic.hpp>

#include <boost/array.hpp>

using namespace boost::unit_test;
using namespace boost::numeric::odeint;

namespace fusion = boost::fusion;

typedef double value_type;
typedef boost::array< value_type , 2 > state_type;

void sys( const state_type &x , state_type &dxdt , const value_type &t )
{
    dxdt[ 0 ] = x[ 0 ] + 2 * x[ 1 ];
    dxdt[ 1 ] = x[ 1 ];
}

typedef explicit_generic_rk< 4 , 4 , state_type> rk_generic_type;
typedef runge_kutta4< state_type > rk4_generic_type;

const boost::array< double , 1 > a1 = {{ 0.5 }};
const boost::array< double , 2 > a2 = {{ 0.0 , 0.5 }};
const boost::array< double , 3 > a3 = {{ 0.0 , 0.0 , 1.0 }};

const rk_generic_type::coef_a_type a = fusion::make_vector( a1 , a2 , a3 );
const rk_generic_type::coef_b_type b = {{ 1.0/6 , 1.0/3 , 1.0/3 , 1.0/6 }};
const rk_generic_type::coef_c_type c = {{ 0.0 , 0.5 , 0.5 , 1.0 }};

typedef runge_kutta4_classic< state_type > rk4_type;

BOOST_AUTO_TEST_SUITE( generic_stepper_test )

BOOST_AUTO_TEST_CASE( test_generic_stepper )
{
    //simultaneously test copying
    rk_generic_type rk_generic_( a , b , c );
    rk_generic_type rk_generic = rk_generic_;

    rk4_generic_type rk4_generic_;
    rk4_generic_type rk4_generic = rk4_generic_;

    //std::cout << stepper;

    rk4_type rk4_;
    rk4_type rk4 = rk4_;

    typedef rk_generic_type::state_type state_type;
    typedef rk_generic_type::value_type stepper_value_type;
    typedef rk_generic_type::deriv_type deriv_type;
    typedef rk_generic_type::time_type time_type;

    state_type x = {{ 0.0 , 1.0 }};
    state_type y = x;
    state_type z = x;

    rk_generic.do_step( sys , x , 0.0 , 0.1 );

    rk4_generic.do_step( sys , y , 0.0 , 0.1 );

    rk4.do_step( sys , z , 0.0 , 0.1 );

    BOOST_CHECK_NE( 0.0 , x[0] );
    BOOST_CHECK_NE( 1.0 , x[1] );
    // compare with analytic solution of above system
    BOOST_CHECK_EQUAL( x[0] , y[0] );
    BOOST_CHECK_EQUAL( x[1] , y[1] );
    BOOST_CHECK_EQUAL( x[0] , z[0] );
    BOOST_CHECK_EQUAL( x[1] , z[1] );

}

BOOST_AUTO_TEST_SUITE_END()
