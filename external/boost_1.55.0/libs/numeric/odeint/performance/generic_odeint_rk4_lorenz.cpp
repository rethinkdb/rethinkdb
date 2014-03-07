/*
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#include <boost/array.hpp>

//#include <boost/numeric/odeint/stepper/explicit_generic_rk.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta4.hpp>
#include <boost/numeric/odeint/algebra/array_algebra.hpp>

#include "rk_performance_test_case.hpp"

#include "lorenz.hpp"

using namespace boost::numeric::odeint;

typedef boost::array< double , 3 > state_type;

/*
typedef explicit_generic_rk< 4 , 4 , state_type , double , state_type , double , array_algebra > rk4_type;

typedef rk4_type::coef_a_type coef_a_type;
typedef rk4_type::coef_b_type coef_b_type;
typedef rk4_type::coef_c_type coef_c_type;

const boost::array< double , 1 > a1 = {{ 0.5 }};
const boost::array< double , 2 > a2 = {{ 0.0 , 0.5 }};
const boost::array< double , 3 > a3 = {{ 0.0 , 0.0 , 1.0 }};

const coef_a_type a = fusion::make_vector( a1 , a2 , a3 );
const coef_b_type b = {{ 1.0/6 , 1.0/3 , 1.0/3 , 1.0/6 }};
const coef_c_type c = {{ 0.0 , 0.5 , 0.5 , 1.0 }};
*/

typedef runge_kutta4< state_type , double , state_type , double , array_algebra > rk4_type;


class rk4_wrapper
{

public:

    rk4_wrapper()
    //        : m_stepper( a , b , c ) 
    {}

    void reset_init_cond()
    {
        m_x[0] = 10.0 * rand() / RAND_MAX;
        m_x[1] = 10.0 * rand() / RAND_MAX;
        m_x[2] = 10.0 * rand() / RAND_MAX;
        m_t = 0.0;
    }

    inline void do_step( const double dt )
    {
        m_stepper.do_step( lorenz(), m_x , m_t , dt );
    }

    double state( const size_t i ) const
    { return m_x[i]; }

private:
    state_type m_x;
    double m_t;
    rk4_type m_stepper;
};



int main()
{
    srand( 12312354 );

    rk4_wrapper stepper;

    run( stepper );
}
