/*
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <cmath>

#include <boost/array.hpp>

template< size_t N >
struct phase_lattice
{
    typedef double value_type;
    typedef boost::array< value_type , N > state_type;

    value_type m_epsilon; 
    state_type m_omega;

    phase_lattice() : m_epsilon( 6.0/(N*N) ) // should be < 8/N^2 to see phase locking
    {
        for( size_t i=1 ; i<N-1 ; ++i )
            m_omega[i] = m_epsilon*(N-i);
    }

    void inline operator()( const state_type &x , state_type &dxdt , const double t ) const
    {
        double c = 0.0;

        for( size_t i=0 ; i<N-1 ; ++i )
        {
            dxdt[i] = m_omega[i] + c;
            c = ( x[i+1] - x[i] );
            dxdt[i] += c;
        }

        //dxdt[N-1] = m_omega[N-1] + sin( x[N-1] - x[N-2] );
    }

};
