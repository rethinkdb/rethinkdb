/*
 * chaotic_system.cpp
 *
 * This example demonstrates how one can use odeint to determine the Lyapunov
 * exponents of a chaotic system namely the well known Lorenz system. Furthermore,
 * it shows how odeint interacts with boost.range.
 *
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#include <iostream>
#include <boost/array.hpp>

#include <boost/numeric/odeint.hpp>

#include "gram_schmidt.hpp"

using namespace std;
using namespace boost::numeric::odeint;


const double sigma = 10.0;
const double R = 28.0;
const double b = 8.0 / 3.0;

//[ system_function_without_perturbations
struct lorenz
{
    template< class State , class Deriv >
    void operator()( const State &x_ , Deriv &dxdt_ , double t ) const
    {
        typename boost::range_iterator< const State >::type x = boost::begin( x_ );
        typename boost::range_iterator< Deriv >::type dxdt = boost::begin( dxdt_ );

        dxdt[0] = sigma * ( x[1] - x[0] );
        dxdt[1] = R * x[0] - x[1] - x[0] * x[2];
        dxdt[2] = -b * x[2] + x[0] * x[1];
    }
};
//]



//[ system_function_with_perturbations
const size_t n = 3;
const size_t num_of_lyap = 3;
const size_t N = n + n*num_of_lyap;

typedef boost::array< double , N > state_type;
typedef boost::array< double , num_of_lyap > lyap_type;

void lorenz_with_lyap( const state_type &x , state_type &dxdt , double t )
{
    lorenz()( x , dxdt , t );

    for( size_t l=0 ; l<num_of_lyap ; ++l )
    {
        const double *pert = x.begin() + 3 + l * 3;
        double *dpert = dxdt.begin() + 3 + l * 3;
        dpert[0] = - sigma * pert[0] + 10.0 * pert[1];
        dpert[1] = ( R - x[2] ) * pert[0] - pert[1] - x[0] * pert[2];
        dpert[2] = x[1] * pert[0] + x[0] * pert[1] - b * pert[2];
    }
}
//]





int main( int argc , char **argv )
{
    state_type x;
    lyap_type lyap;
    runge_kutta4< state_type > rk4;

    fill( x.begin() , x.end() , 0.0 );
    x[0] = 10.0 ; x[1] = 10.0 ; x[2] = 5.0;

    const double dt = 0.01;

    //[ integrate_transients_with_range
    // perform 10000 transient steps
    integrate_n_steps( rk4 , lorenz() , std::make_pair( x.begin() , x.begin() + n ) , 0.0 , dt , 10000 );
    //]

    //[ lyapunov_full_code
    fill( x.begin()+n , x.end() , 0.0 );
    for( size_t i=0 ; i<num_of_lyap ; ++i ) x[n+n*i+i] = 1.0;
    fill( lyap.begin() , lyap.end() , 0.0 );

    double t = 0.0;
    size_t count = 0;
    while( true )
    {

        t = integrate_n_steps( rk4 , lorenz_with_lyap , x , t , dt , 100 );
        gram_schmidt< num_of_lyap >( x , lyap , n );
        ++count;

        if( !(count % 100000) )
        {
            cout << t;
            for( size_t i=0 ; i<num_of_lyap ; ++i ) cout << "\t" << lyap[i] / t ;
            cout << endl;
        }
    }
    //]

    return 0;
}
