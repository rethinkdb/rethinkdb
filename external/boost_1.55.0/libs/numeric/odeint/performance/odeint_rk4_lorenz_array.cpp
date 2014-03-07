/*
 * odeint_rk4_lorenz.cpp
 *
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#include <boost/array.hpp>

#include <boost/numeric/odeint/stepper/runge_kutta4_classic.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta4.hpp>
#include <boost/numeric/odeint/algebra/array_algebra.hpp>

#include "rk_performance_test_case.hpp"

#include "lorenz.hpp"

typedef boost::array< double , 3 > state_type;
typedef boost::numeric::odeint::runge_kutta4< state_type , double , state_type , double ,
                                              boost::numeric::odeint::array_algebra > rk4_odeint_type;


class odeint_wrapper
{
public:
    void reset_init_cond()
    {
        /* random */
        /*
        m_x[0] = 10.0 * rand() / RAND_MAX;
        m_x[1] = 10.0 * rand() / RAND_MAX;
        m_x[2] = 10.0 * rand() / RAND_MAX;
        */
        /* hand chosen random (cf fortran) */
        m_x[0] = 8.5;
        m_x[1] = 3.1;
        m_x[2] = 1.2;

        m_t = 0.0;
    }

    inline void do_step( const double dt )
    {
        m_stepper.do_step( lorenz() , m_x , m_t , dt );
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

    run( stepper );
}
