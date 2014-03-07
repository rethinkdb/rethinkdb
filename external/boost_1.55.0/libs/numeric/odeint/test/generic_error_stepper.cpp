/*
 [auto_generated]
 libs/numeric/odeint/test/generic_error_stepper.cpp

 [begin_description]
 This file tests the generic error stepper.
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

#define BOOST_TEST_MODULE odeint_generic_error_stepper

#include <iostream>
#include <utility>

#include <boost/test/unit_test.hpp>

#include <boost/numeric/odeint/stepper/explicit_error_generic_rk.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta_cash_karp54.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta_cash_karp54_classic.hpp>

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

typedef explicit_error_generic_rk< 6 , 5 , 4 , 5 , state_type> error_rk_generic_type;

const boost::array< double , 1 > a1 = {{ 0.2 }};
const boost::array< double , 2 > a2 = {{ 3.0/40.0 , 9.0/40 }};
const boost::array< double , 3 > a3 = {{ 0.3 , -0.9 , 1.2 }};
const boost::array< double , 4 > a4 = {{ -11.0/54.0 , 2.5 , -70.0/27.0 , 35.0/27.0 }};
const boost::array< double , 5 > a5 = {{ 1631.0/55296.0 , 175.0/512.0 , 575.0/13824.0 , 44275.0/110592.0 , 253.0/4096.0 }};

const error_rk_generic_type::coef_a_type a = fusion::make_vector( a1 , a2 , a3 , a4 , a5 );
const error_rk_generic_type::coef_b_type b = {{ 37.0/378.0 , 0.0 , 250.0/621.0 , 125.0/594.0 , 0.0 , 512.0/1771.0 }};
const error_rk_generic_type::coef_b_type b2 = {{ b[0]-2825.0/27648.0 , b[1]-0.0 , b[2]-18575.0/48384.0 , b[3]-13525.0/55296.0 , b[4]-277.0/14336.0 , b[5]-0.25 }};
const error_rk_generic_type::coef_c_type c = {{ 0.0 , 0.2 , 0.3 , 0.6 , 1.0 , 7.0/8 }};

typedef runge_kutta_cash_karp54< state_type > error_rk54_ck_generic_type;
typedef runge_kutta_cash_karp54_classic< state_type > rk54_ck_type;

BOOST_AUTO_TEST_SUITE( generic_error_stepper_test )

BOOST_AUTO_TEST_CASE( test_generic_error_stepper )
{
    //simultaneously test copying
    error_rk_generic_type rk_generic_( a , b , b2 , c );
    error_rk_generic_type rk_generic = rk_generic_;

    error_rk54_ck_generic_type rk54_ck_generic_;
    error_rk54_ck_generic_type rk54_ck_generic = rk54_ck_generic_;

    //std::cout << stepper;

    rk54_ck_type rk54_ck_;
    rk54_ck_type rk54_ck = rk54_ck_;

    typedef error_rk_generic_type::state_type state_type;
    typedef error_rk_generic_type::value_type stepper_value_type;
    typedef error_rk_generic_type::deriv_type deriv_type;
    typedef error_rk_generic_type::time_type time_type;

    state_type x = {{ 0.0 , 1.0 }};
    state_type y = x;
    state_type z = x;
    state_type x_err , y_err , z_err;

    rk_generic.do_step( sys , x , 0.0 , 0.1 , x_err );

    rk54_ck_generic.do_step( sys , y , 0.0 , 0.1 , y_err);

    rk54_ck.do_step( sys , z , 0.0 , 0.1 , z_err );

    BOOST_CHECK_NE( 0.0 , x[0] );
    BOOST_CHECK_NE( 1.0 , x[1] );
    BOOST_CHECK_NE( 0.0 , x_err[0] );
    BOOST_CHECK_NE( 0.0 , x_err[1] );
    // compare with analytic solution of above system
    BOOST_CHECK_EQUAL( x[0] , y[0] );
    BOOST_CHECK_EQUAL( x[1] , y[1] );
    BOOST_CHECK_EQUAL( x[0] , z[0] );
    BOOST_CHECK_EQUAL( x[1] , z[1] );

}

BOOST_AUTO_TEST_SUITE_END()
