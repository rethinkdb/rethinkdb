/*
 * bulirsch_stoer.cpp
 *
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <iostream>
#include <fstream>
#define _USE_MATH_DEFINES
#include <cmath>

#include <boost/array.hpp>
#include <boost/ref.hpp>

#include <boost/numeric/odeint/config.hpp>

#include <boost/numeric/odeint.hpp>
#include <boost/numeric/odeint/stepper/bulirsch_stoer.hpp>
#include <boost/numeric/odeint/stepper/bulirsch_stoer_dense_out.hpp>

using namespace std;
using namespace boost::numeric::odeint;

typedef boost::array< double , 1 > state_type;

/*
 * x' = ( - x*sin t  + 2 tan x ) y
 * with x( pi/6 ) = 2/sqrt(3) the analytic solution is 1/cos t
 */

void rhs( const state_type &x , state_type &dxdt , const double t )
{
    dxdt[0] = ( - x[0] * sin( t ) + 2.0 * tan( t ) ) * x[0];
}

void rhs2( const state_type &x , state_type &dxdt , const double t )
{
    dxdt[0] = sin(t);
}


ofstream out;

void write_out( const state_type &x , const double t )
{
    out << t << '\t' << x[0] << endl;
}

int main()
{
    bulirsch_stoer_dense_out< state_type > stepper( 1E-8 , 0.0 , 0.0 , 0.0 );
    bulirsch_stoer< state_type > stepper2( 1E-8 , 0.0 , 0.0 , 0.0 );

    state_type x = {{ 2.0 / sqrt(3.0) }};

    double t = M_PI/6.0;
    //double t = 0.0;
    double dt = 0.01;
    double t_end = M_PI/2.0 - 0.1;
    //double t_end = 100.0;

    out.open( "bs.dat" );
    out.precision(16);
    integrate_const( stepper , rhs , x , t , t_end , dt , write_out );
    out.close();

    x[0] = 2.0 / sqrt(3.0);

    out.open( "bs2.dat" );
    out.precision(16);
    integrate_adaptive( stepper , rhs , x , t , t_end , dt , write_out );
    out.close();

    x[0] = 2.0 / sqrt(3.0);

    out.open( "bs3.dat" );
    out.precision(16);
    integrate_adaptive( stepper2 , rhs , x , t , t_end , dt , write_out );
    out.close();


    typedef runge_kutta_dopri5< state_type > dopri5_type;
    typedef controlled_runge_kutta< dopri5_type > controlled_dopri5_type;
    typedef dense_output_runge_kutta< controlled_dopri5_type > dense_output_dopri5_type;

    dense_output_dopri5_type dopri5( controlled_dopri5_type( default_error_checker< double >( 1E-2 , 0.0 , 0.0 , 0.0 )  ) );

    x[0] = 2.0 / sqrt(3.0);

    out.open( "bs4.dat" );
    out.precision(16);
    integrate_adaptive( dopri5 , rhs , x , t , t_end , dt , write_out );
    out.close();

}
