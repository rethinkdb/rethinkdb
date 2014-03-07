/*
 * rt_explicit_rk.hpp
 *
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef RT_EXPLICIT_RK_HPP_
#define RT_EXPLICIT_RK_HPP_

#include <vector>

#include "rt_algebra.hpp"

using namespace std;

template< class StateType >
class rt_explicit_rk
{
public:
    typedef StateType state_type;
    typedef double* const * coeff_a_type;
    typedef vector< double > coeff_b_type;
    typedef vector< double > coeff_c_type;

    rt_explicit_rk( size_t stage_count ) : m_s( stage_count )
    {
        m_F = new state_type[ m_s ];
    }

    rt_explicit_rk( const size_t stage_count , 
                    const coeff_a_type a , 
                    const coeff_b_type &b , const coeff_c_type &c )
        : m_s( stage_count ) , m_a( a ) , m_b( b ) , m_c( c )
    {
        m_F = new state_type[ m_s ];
    }

    ~rt_explicit_rk()
    {
        delete[] m_F;
    }

    /* void set_params( coeff_a_type &a , coeff_b_type &b , coeff_c_type &c )
    {
        m_a = a;
        m_b = b;
        m_c = c;
        }*/

    template< class System >
    void do_step( System sys , state_type &x , const double t , const double dt )
    {
        // first stage separately
        sys( x , m_F[0] , t + m_c[0]*t );
        if( m_s == 1 )
            rt_algebra::foreach( x , x , &m_b[0] , m_F , dt , 1 );
        else
            rt_algebra::foreach( m_x_tmp , x , m_a[0] , m_F , dt , 1 );

        for( size_t stage = 2 ; stage <= m_s ; ++stage )
        {
            sys( m_x_tmp , m_F[stage-1] , t + m_c[stage-1]*dt );
            if( stage == m_s )
                rt_algebra::foreach( x , x , &m_b[0] , m_F , dt , stage-1 );
            else
                rt_algebra::foreach( m_x_tmp , x , m_a[stage-1] , m_F , dt , stage-1 );
        }
    }


private:
    const size_t m_s;
    const coeff_a_type m_a;
    const coeff_b_type m_b;
    const coeff_c_type m_c;

    state_type m_x_tmp;
    state_type *m_F;
};

#endif /* RT_EXPLICIT_RK_HPP_ */
