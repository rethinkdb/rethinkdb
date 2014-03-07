/* Boost numeric test of the runge kutta steppers test file

 Copyright 2012 Karsten Ahnert
 Copyright 2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
*/

// disable checked iterator warning for msvc
#include <boost/config.hpp>
#ifdef BOOST_MSVC
    #pragma warning(disable:4996)
#endif

#define BOOST_TEST_MODULE numeric_runge_kutta

#include <iostream>
#include <cmath>

#include <boost/array.hpp>

#include <boost/test/unit_test.hpp>

#include <boost/mpl/vector.hpp>

#include <boost/numeric/odeint.hpp>

using namespace boost::unit_test;
using namespace boost::numeric::odeint;
namespace mpl = boost::mpl;

typedef double value_type;

typedef boost::array< double , 2 > state_type;

// harmonic oscillator, analytic solution x[0] = sin( t )
struct osc
{
    void operator()( const state_type &x , state_type &dxdt , const double t ) const
    {
        dxdt[0] = x[1];
        dxdt[1] = -x[0];
    }
};

/* reset dispatcher */
template< class StepperCategory >
struct resetter
{
    template< class Stepper >
    static void reset( Stepper &stepper ) { }
};

template< >
struct resetter< explicit_error_stepper_fsal_tag >
{
    template< class Stepper >
    static void reset( Stepper &stepper ) 
    { stepper.reset(); }
};


BOOST_AUTO_TEST_SUITE( numeric_runge_kutta_test )


/* generic test for all runge kutta steppers */
template< class Stepper >
struct perform_runge_kutta_test
{
    void operator()( void )
    {
   
        Stepper stepper;
        const int o = stepper.order()+1; //order of the error is order of approximation + 1

        const state_type x0 = {{ 0.0 , 1.0 }};
        state_type x1;
        const double t = 0.0;
        /* do a first step with dt=0.1 to get an estimate on the prefactor of the error dx = f * dt^(order+1) */
        double dt = 0.5;
        stepper.do_step( osc() , x0 , t , x1 , dt );
        const double f = 2.0 * std::abs( sin(dt) - x1[0] ) / std::pow( dt , o ); // upper bound
        
        std::cout << o << " , " << f << std::endl;

        /* as long as we have errors above machine precision */
        while( f*std::pow( dt , o ) > 1E-16 )
        {
            // reset stepper which require resetting (fsal steppers)
            resetter< typename Stepper::stepper_category >::reset( stepper );
            
            stepper.do_step( osc() , x0 , t , x1 , dt );
            std::cout << "Testing dt=" << dt << std::endl;
            BOOST_CHECK_LT( std::abs( sin(dt) - x1[0] ) , f*std::pow( dt , o ) );
            dt *= 0.5;
        }
    }
};


/* generic error test for all runge kutta steppers */
template< class Stepper >
struct perform_runge_kutta_error_test
{
    void operator()( void )
    {
        Stepper stepper;
        const int o = stepper.error_order()+1; //order of the error is order of approximation + 1

        const state_type x0 = {{ 0.0 , 1.0 }};
        state_type x1 , x_err;
        const double t = 0.0;
        /* do a first step with dt=0.1 to get an estimate on the prefactor of the error dx = f * dt^(order+1) */
        double dt = 0.5;
        stepper.do_step( osc() , x0 , t , x1 , dt , x_err );
        const double f = 2.0 * std::abs( x_err[0] ) / std::pow( dt , o );

        std::cout << o << " , " << f << " , " << x0[0] << std::endl;

        /* as long as we have errors above machine precision */
        while( f*std::pow( dt , o ) > 1E-16 )
        {
            // reset stepper which require resetting (fsal steppers)
            resetter< typename Stepper::stepper_category >::reset( stepper );
            
            stepper.do_step( osc() , x0 , t , x1 , dt , x_err );
            std::cout << "Testing dt=" << dt << ": " << x_err[1] << std::endl;
            BOOST_CHECK_SMALL( std::abs( x_err[0] ) , f*std::pow( dt , o ) );
            dt *= 0.5;
        }
    }
};


typedef mpl::vector<
    euler< state_type > ,
    modified_midpoint< state_type > ,
    runge_kutta4< state_type > ,
    runge_kutta4_classic< state_type > ,
    runge_kutta_cash_karp54_classic< state_type > ,
    runge_kutta_cash_karp54< state_type > ,
    runge_kutta_dopri5< state_type > ,
    runge_kutta_fehlberg78< state_type >
    > runge_kutta_steppers;

BOOST_AUTO_TEST_CASE_TEMPLATE( runge_kutta_test , Stepper, runge_kutta_steppers )
{
    perform_runge_kutta_test< Stepper > tester;
    tester();
}


typedef mpl::vector<
    runge_kutta_cash_karp54_classic< state_type > ,
    runge_kutta_cash_karp54< state_type > ,
    runge_kutta_dopri5< state_type > ,
    runge_kutta_fehlberg78< state_type >
    > runge_kutta_error_steppers;

BOOST_AUTO_TEST_CASE_TEMPLATE( runge_kutta_error_test , Stepper, runge_kutta_error_steppers )
{
    perform_runge_kutta_error_test< Stepper > tester;
    tester();
}

BOOST_AUTO_TEST_SUITE_END()
