/*
 * lorenz_gmpxx.cpp
 *
 * This example demonstrates how odeint can be used with arbitrary precision types.
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

#include <gmpxx.h>

#include <boost/numeric/odeint.hpp>

using namespace std;
using namespace boost::numeric::odeint;

//[ gmpxx_lorenz
typedef mpf_class value_type;
typedef boost::array< value_type , 3 > state_type;

struct lorenz
{
    void operator()( const state_type &x , state_type &dxdt , value_type t ) const
    {
        const value_type sigma( 10.0 );
        const value_type R( 28.0 );
        const value_type b( value_type( 8.0 ) / value_type( 3.0 ) );

        dxdt[0] = sigma * ( x[1] - x[0] );
        dxdt[1] = R * x[0] - x[1] - x[0] * x[2];
        dxdt[2] = -b * x[2] + x[0] * x[1];
    }
};
//]




struct streaming_observer
{
    std::ostream& m_out;

    streaming_observer( std::ostream &out ) : m_out( out ) { }

    template< class State , class Time >
    void operator()( const State &x , Time t ) const
    {
        m_out << t;
        for( size_t i=0 ; i<x.size() ; ++i ) m_out << "\t" << x[i] ;
        m_out << "\n";
    }
};






int main( int argc , char **argv )
{
    //[ gmpxx_integration
    const int precision = 1024;
    mpf_set_default_prec( precision );

    state_type x = {{ value_type( 10.0 ) , value_type( 10.0 ) , value_type( 10.0 ) }};

    cout.precision( 1000 );
    integrate_const( runge_kutta4< state_type , value_type >() ,
            lorenz() , x , value_type( 0.0 ) , value_type( 10.0 ) , value_type( value_type( 1.0 ) / value_type( 10.0 ) ) ,
            streaming_observer( cout ) );
    //]

    return 0;
}
