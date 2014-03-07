/* Boost check_gmp.cpp test file

 Copyright 2009 Karsten Ahnert
 Copyright 2009 Mario Mulansky

 This file tests the odeint library with the gmp arbitrary precision types

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#define BOOST_TEST_MODULE odeint_gmp

#include <gmpxx.h>

#include <boost/test/unit_test.hpp>
#include <boost/array.hpp>

#include <boost/numeric/odeint/stepper/runge_kutta4.hpp>

using namespace boost::unit_test;
using namespace boost::numeric::odeint;

const int precision = 1024;

typedef mpf_class value_type;
typedef boost::array< value_type , 1 > state_type;


void constant_system( state_type &x , state_type &dxdt , value_type t )
{
    dxdt[0] = value_type( 1.0 , precision );
}


BOOST_AUTO_TEST_CASE( gmp )
{
    /* We have to specify the desired precision in advance! */
    mpf_set_default_prec( precision );

    mpf_t eps_ , unity;
    mpf_init( eps_ ); mpf_init( unity );
    mpf_set_d( unity , 1.0 );
    mpf_div_2exp( eps_ , unity , precision-1 ); // 2^(-precision+1) : smallest number that can be represented with used precision
    value_type eps( eps_ );

    runge_kutta4< state_type , value_type > stepper;
    state_type x;
    x[0] = 0.0;

    stepper.do_step( constant_system , x , 0.0 , 0.1 );

    BOOST_MESSAGE( eps );
    BOOST_CHECK_MESSAGE( abs( x[0] - value_type( 0.1 , precision ) ) < eps , x[0] - 0.1 );
}
