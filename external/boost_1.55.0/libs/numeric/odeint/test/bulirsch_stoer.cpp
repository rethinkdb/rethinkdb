/*
 [auto_generated]
 libs/numeric/odeint/test/bulirsch_stoer.cpp

 [begin_description]
 This file tests the Bulirsch-Stoer stepper.
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

#define BOOST_TEST_MODULE odeint_bulirsch_stoer

#include <utility>
#include <iostream>

#include <boost/array.hpp>

#include <boost/test/unit_test.hpp>

#include <boost/numeric/odeint/stepper/bulirsch_stoer.hpp>
#include <boost/numeric/odeint/stepper/bulirsch_stoer_dense_out.hpp>

#include <boost/numeric/odeint/integrate/integrate_adaptive.hpp>

using namespace boost::unit_test;
using namespace boost::numeric::odeint;

typedef double value_type;
typedef boost::array< value_type , 3 > state_type;

const double sigma = 10.0;
const double R = 28.0;
const double b = 8.0 / 3.0;

struct lorenz
{
    template< class State , class Deriv >
    void operator()( const State &x , Deriv &dxdt , double t ) const
    {
        dxdt[0] = sigma * ( x[1] - x[0] );
        dxdt[1] = R * x[0] - x[1] - x[0] * x[2];
        dxdt[2] = -b * x[2] + x[0] * x[1];
    }
};

struct const_system
{
    template< class State , class Deriv >
    void operator()( const State &x , Deriv &dxdt , double t ) const
    {
        dxdt[0] = 1.0;
        dxdt[1] = 1.0;
        dxdt[2] = 1.0;
    }
};

struct sin_system
{
    template< class State , class Deriv >
    void operator()( const State &x , Deriv &dxdt , double t ) const
    {
        dxdt[0] = sin( x[0] );
        dxdt[1] = cos( x[1] );
        dxdt[2] = sin( x[2] ) + cos( x[2] );
    }
};

BOOST_AUTO_TEST_SUITE( bulirsch_stoer_test )

BOOST_AUTO_TEST_CASE( test_bulirsch_stoer )
{
    typedef bulirsch_stoer< state_type > stepper_type;
    stepper_type stepper( 1E-9 , 1E-9 , 1.0 , 0.0 );

    state_type x;
    x[0] = 10.0 ; x[1] = 10.0 ; x[2] = 5.0;

    double dt = 0.1;

    //stepper.try_step( lorenz() , x , t , dt );

    std::cout << "starting integration..." << std::endl;

    size_t steps = integrate_adaptive( stepper , lorenz() , x , 0.0 , 10.0 , dt );

    std::cout << "required steps: " << steps << std::endl;

    bulirsch_stoer_dense_out< state_type > bs_do( 1E-9 , 1E-9 , 1.0 , 0.0 );
    x[0] = 10.0 ; x[1] = 10.0 ; x[2] = 5.0;
    double t = 0.0;
    dt = 1E-1;
    bs_do.initialize( x , t , dt );
    bs_do.do_step( sin_system() );
    std::cout << "one step successful, new time: " << bs_do.current_time() << " (" << t << ")" << std::endl;

    x = bs_do.current_state();
    std::cout << "x( " << bs_do.current_time() << " ) = [ " << x[0] << " , " << x[1] << " , " << x[2] << " ]" << std::endl;

    bs_do.calc_state( bs_do.current_time()/3 , x );
    std::cout << "x( " << bs_do.current_time()/3 << " ) = [ " << x[0] << " , " << x[1] << " , " << x[2] << " ]" << std::endl;

    std::cout << std::endl << "=======================================================================" << std::endl << std::endl;

    x[0] = 10.0 ; x[1] = 10.0 ; x[2] = 5.0;
    t = 0.0; dt /= 3;
    bs_do.initialize( x , t , dt );
    bs_do.do_step( sin_system() );
    x = bs_do.current_state();
    std::cout << "x( " << bs_do.current_time() << " ) = [ " << x[0] << " , " << x[1] << " , " << x[2] << " ]" << std::endl;

    t = dt;
    bs_do.initialize( x , t , dt );
    bs_do.do_step( sin_system() );
    x = bs_do.current_state();

    t = 2*dt;
    bs_do.initialize( x , t , dt );
    bs_do.do_step( sin_system() );
    x = bs_do.current_state();

    std::cout << "x( " << bs_do.current_time() << " ) = [ " << x[0] << " , " << x[1] << " , " << x[2] << " ]" << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
