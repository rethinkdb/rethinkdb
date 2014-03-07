/*
 * gsl_rk4_lorenz.cpp
 *
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#include <gsl/gsl_odeiv.h>

#include "rk_performance_test_case.hpp"

#include "lorenz_gsl.hpp"

const size_t dim = 3;

class gsl_wrapper
{
public:

    gsl_wrapper()
    {
        m_s = gsl_odeiv_step_alloc( gsl_odeiv_step_rk4 , dim);
        m_sys.function = lorenz_gsl;
        m_sys.jacobian = 0;
        m_sys.dimension = dim;
        m_sys.params = 0;
    }

    void reset_init_cond()
    {
        m_x[0] = 10.0 * rand() / RAND_MAX;
        m_x[1] = 10.0 * rand() / RAND_MAX;
        m_x[2] = 10.0 * rand() / RAND_MAX;
        m_t = 0.0;
    }

    inline void do_step( const double dt )
    {
        gsl_odeiv_step_apply ( m_s , m_t , dt , m_x , m_x_err , 0 , 0 , &m_sys );
        //m_t += dt;
    }

    double state( const size_t i ) const
    { return m_x[i]; }

    ~gsl_wrapper()
    {
        gsl_odeiv_step_free( m_s );
    }

private:
    double m_x[dim];
    double m_x_err[dim];
    double m_t;
    gsl_odeiv_step *m_s;
    gsl_odeiv_system m_sys;
};



int main()
{
    gsl_wrapper stepper;

    run( stepper , 20000000 / 3 , 1E-10 * 3);
}
