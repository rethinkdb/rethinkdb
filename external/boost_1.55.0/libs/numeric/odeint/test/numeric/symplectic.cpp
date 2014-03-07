/* Boost numeric test of the symplectic steppers test file

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

#define BOOST_TEST_MODULE numeric_symplectic

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

typedef boost::array< double ,1 > state_type;

// harmonic oscillator, analytic solution x[0] = sin( t )
struct osc
{
    void operator()( const state_type &q , state_type &dpdt ) const
    {
        dpdt[0] = -q[0];
    }
};

BOOST_AUTO_TEST_SUITE( numeric_symplectic_test )


/* generic test for all symplectic steppers */
template< class Stepper >
struct perform_symplectic_test
{
    void operator()( void )
    {
   
        Stepper stepper;
        const int o = stepper.order()+1; //order of the error is order of approximation + 1

        const state_type q0 = {{ 0.0 }};
        const state_type p0 = {{ 1.0 }};
        state_type q1,p1;
        std::pair< state_type , state_type >x1( q1 , p1 );
        const double t = 0.0;
        /* do a first step with dt=0.1 to get an estimate on the prefactor of the error dx = f * dt^(order+1) */
        double dt = 0.5;
        stepper.do_step( osc() , std::make_pair( q0 , p0 ) , t , x1 , dt );
        const double f = 2.0 * std::abs( sin(dt) - x1.first[0] ) / std::pow( dt , o );

        std::cout << o << " , " << f << std::endl;

        /* as long as we have errors above machine precision */
        while( f*std::pow( dt , o ) > 1E-16 )
        {
            stepper.do_step( osc() , std::make_pair( q0 , p0 ) , t , x1 , dt );
            std::cout << "Testing dt=" << dt << std::endl;
            BOOST_CHECK_SMALL( std::abs( sin(dt) - x1.first[0] ) , f*std::pow( dt , o ) );
            dt *= 0.5;
        }
    }
};


typedef mpl::vector<
    symplectic_euler< state_type > ,
    symplectic_rkn_sb3a_mclachlan< state_type >
    > symplectic_steppers;

BOOST_AUTO_TEST_CASE_TEMPLATE( symplectic_test , Stepper, symplectic_steppers )
{
    perform_symplectic_test< Stepper > tester;
    tester();
}

BOOST_AUTO_TEST_SUITE_END()
