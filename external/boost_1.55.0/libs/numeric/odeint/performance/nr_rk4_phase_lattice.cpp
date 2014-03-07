/*
 * nr_rk4_phase_lattice.cpp
 *
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#include <boost/array.hpp>

#include "rk_performance_test_case.hpp"

#include "phase_lattice.hpp"

const size_t dim = 1024;

typedef boost::array< double , dim > state_type;


template< class System , typename T , size_t dim >
void rk4_step( const System sys , boost::array< T , dim > &x , const double t , const double dt )
{   // fast rk4 implementation adapted from the book 'Numerical Recipes'
    size_t i;
    const double hh = dt*0.5;
    const double h6 = dt/6.0;
    const double th = t+hh;
    boost::array< T , dim > dydx , dym , dyt , yt;

    sys( x , dydx , t );

    for( i=0 ; i<dim ; i++ )
        yt[i] = x[i] + hh*dydx[i];

    sys( yt , dyt , th );
    for( i=0 ; i<dim ; i++ )
        yt[i] = x[i] + hh*dyt[i];

    sys( yt , dym , th );
    for( i=0 ; i<dim ; i++ ) {
        yt[i] = x[i] + dt*dym[i];
        dym[i] += dyt[i];
    }
    sys( yt , dyt , t+dt );
    for( i=0 ; i<dim ; i++ )
        x[i] += h6*( dydx[i] + dyt[i] + 2.0*dym[i] );
}


class nr_wrapper
{
public:
    void reset_init_cond()
    {
        for( size_t i = 0 ; i<dim ; ++i )
            m_x[i] = 2.0*3.1415927*rand() / RAND_MAX;
        m_t = 0.0;
    }

    inline void do_step( const double dt )
    {
        rk4_step( phase_lattice<dim>() , m_x , m_t , dt );
    }

    double state( const size_t i ) const
    { return m_x[i]; }

private:
    state_type m_x;
    double m_t;
};



int main()
{
    srand( 12312354 );

    nr_wrapper stepper;

    run( stepper , 10000 , 1E-6 );
}
