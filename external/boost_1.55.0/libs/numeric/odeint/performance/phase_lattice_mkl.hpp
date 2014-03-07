/*
 * phase_lattice_mkl.hpp
 *
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */




#ifndef PHASE_LATTICE_MKL_HPP_
#define PHASE_LATTICE_MKL_HPP_

#include <cmath>

#include <mkl_blas.h>
#include <mkl_vml_functions.h>
#include <boost/array.hpp>

template< size_t N >
struct phase_lattice_mkl
{
    typedef double value_type;
    typedef boost::array< value_type , N > state_type;

    value_type m_epsilon;
    state_type m_omega;
    state_type m_tmp;

    phase_lattice_mkl() : m_epsilon( 6.0/(N*N) ) // should be < 8/N^2 to see phase locking
    {
        for( size_t i=1 ; i<N-1 ; ++i )
            m_omega[i] = m_epsilon*(N-i);
    }

    void inline operator()( const state_type &x , state_type &dxdt , const double t )
    {
        const int n = x.size();

        dxdt[0] = m_omega[0] + sin( x[1] - x[0] );

        vdSub( n-1 , &(x[1]) , &(x[0]) , &(m_tmp[0]) );
        vdSin( n-1 , &(m_tmp[0]) , &(m_tmp[0]) );
        vdAdd( n-2 , &(m_tmp[0]) , &(m_tmp[1]) , &(dxdt[1]) );
        vdAdd( n-2 , &(dxdt[1]) , &(m_omega[1]) , &(dxdt[1]) );

        dxdt[N-1] = m_omega[N-1] + sin( x[N-1] - x[N-2] );
    }

};


#endif /* PHASE_LATTICE_MKL_HPP_ */
