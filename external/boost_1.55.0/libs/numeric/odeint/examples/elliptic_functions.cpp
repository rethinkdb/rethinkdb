/*
 * elliptic_functions.cpp
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
#include <cmath>

#include <boost/array.hpp>

#include <boost/numeric/odeint/config.hpp>

#include <boost/numeric/odeint.hpp>
#include <boost/numeric/odeint/stepper/bulirsch_stoer.hpp>
#include <boost/numeric/odeint/stepper/bulirsch_stoer_dense_out.hpp>

using namespace std;
using namespace boost::numeric::odeint;

typedef boost::array< double , 3 > state_type;

/*
 * x1' = x2*x3
 * x2' = -x1*x3
 * x3' = -m*x1*x2
 */

void rhs( const state_type &x , state_type &dxdt , const double t )
{
    static const double m = 0.51;

    dxdt[0] = x[1]*x[2];
    dxdt[1] = -x[0]*x[2];
    dxdt[2] = -m*x[0]*x[1];
}

ofstream out;

void write_out( const state_type &x , const double t )
{
    out << t << '\t' << x[0] << '\t' << x[1] << '\t' << x[2] << endl;
}

int main()
{
    bulirsch_stoer_dense_out< state_type > stepper( 1E-9 , 1E-9 , 1.0 , 0.0 );

    state_type x1 = {{ 0.0 , 1.0 , 1.0 }};

    double t = 0.0;
    double dt = 0.01;

    out.open( "elliptic1.dat" );
    out.precision(16);
    integrate_const( stepper , rhs , x1 , t , 100.0 , dt , write_out );
    out.close();

    state_type x2 = {{ 0.0 , 1.0 , 1.0 }};

    out.open( "elliptic2.dat" );
    out.precision(16);
    integrate_adaptive( stepper , rhs , x2 , t , 100.0 , dt , write_out );
    out.close();

    typedef runge_kutta_dopri5< state_type > dopri5_type;
    typedef controlled_runge_kutta< dopri5_type > controlled_dopri5_type;
    typedef dense_output_runge_kutta< controlled_dopri5_type > dense_output_dopri5_type;

    dense_output_dopri5_type dopri5( controlled_dopri5_type( default_error_checker< double >( 1E-9 , 0.0 , 0.0 , 0.0 )  ) );

    state_type x3 = {{ 0.0 , 1.0 , 1.0 }};

    out.open( "elliptic3.dat" );
    out.precision(16);
    integrate_adaptive( dopri5 , rhs , x3 , t , 100.0 , dt , write_out );
    out.close();
}
