/* Boost numeric test of the rosenbrock4 stepper test file

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

#define BOOST_TEST_MODULE numeric_rosenbrock

#include <iostream>
#include <cmath>

#include <boost/array.hpp>

#include <boost/test/unit_test.hpp>

#include <boost/numeric/odeint/stepper/rosenbrock4.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>

using namespace boost::unit_test;
using namespace boost::numeric::odeint;

typedef double value_type;
typedef boost::numeric::ublas::vector< value_type > state_type;
typedef boost::numeric::ublas::matrix< value_type > matrix_type;

// harmonic oscillator, analytic solution x[0] = sin( t )
struct sys
{
    void operator()( const state_type &x , state_type &dxdt , const value_type &t ) const
    {
        dxdt( 0 ) = x( 1 );
        dxdt( 1 ) = -x( 0 );
    }
};

struct jacobi
{
    void operator()( const state_type &x , matrix_type &jacobi , const value_type &t , state_type &dfdt ) const
    {
        jacobi( 0 , 0 ) = 0;
        jacobi( 0 , 1 ) = 1;
        jacobi( 1 , 0 ) = -1;
        jacobi( 1 , 1 ) = 0;
        dfdt( 0 ) = 0.0;
        dfdt( 1 ) = 0.0;
    }
};


BOOST_AUTO_TEST_SUITE( numeric_rosenbrock4 )

BOOST_AUTO_TEST_CASE( rosenbrock4_numeric_test )
{
    typedef rosenbrock4< value_type > stepper_type;
    stepper_type stepper;

    const int o = stepper.order()+1;

    state_type x0( 2 ) , x1( 2 );
    x0(0) = 0.0; x0(1) = 1.0;

    double dt = 0.5;

    stepper.do_step( std::make_pair( sys() , jacobi() ) , x0 , 0.0 , x1 , dt );
    const double f = 2.0 * std::abs( sin(dt) - x1(0) ) / std::pow( dt , o );

    std::cout << o << " , " << f << std::endl;

    while( f*std::pow( dt , o ) > 1E-16 )
    {
        stepper.do_step( std::make_pair( sys() , jacobi() ) , x0 , 0.0 , x1 , dt );
        std::cout << "Testing dt=" << dt << std::endl;
        BOOST_CHECK_SMALL( std::abs( sin(dt) - x1(0) ) , f*std::pow( dt , o ) );
        dt *= 0.5;
    }
}

BOOST_AUTO_TEST_SUITE_END()
