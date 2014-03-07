/*
 * odeint_rk4_phase_lattice_mkl.cpp
 *
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#include <boost/array.hpp>

#include <boost/numeric/odeint/stepper/runge_kutta4.hpp>
#include <boost/numeric/odeint/algebra/vector_space_algebra.hpp>
#include <boost/numeric/odeint/external/mkl/mkl_operations.hpp>


#include "rk_performance_test_case.hpp"

#include "phase_lattice.hpp"
#include "phase_lattice_mkl.hpp"

const size_t N = 1024;

typedef boost::array< double , N > state_type;
typedef boost::numeric::odeint::runge_kutta4< state_type 
                                              , double , state_type , double ,
                                              boost::numeric::odeint::vector_space_algebra , 
                                              boost::numeric::odeint::mkl_operations 
                                              > rk4_odeint_type;


class odeint_wrapper
{
public:
    void reset_init_cond()
    {
        for( size_t i = 0 ; i<N ; ++i )
            m_x[i] = 2.0*3.1415927*rand() / RAND_MAX;
        m_t = 0.0;
    }

    inline void do_step( const double dt )
    {
        m_stepper.do_step( phase_lattice_mkl<N>() , m_x , m_t , dt );
        //m_t += dt;
    }

    double state( const size_t i ) const
    { return m_x[i]; }

private:
    state_type m_x;
    double m_t;
    rk4_odeint_type m_stepper;
};



int main()
{
    srand( 12312354 );

    odeint_wrapper stepper;

    run( stepper , 10000 , 1E-6 );
}
