/*
 [auto_generated]
 libs/numeric/odeint/test/generation.cpp

 [begin_description]
 This file tests the generation functions.
 [end_description]

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#define BOOST_TEST_MODULE odeint_generation

#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/numeric/odeint/config.hpp>
#include <boost/numeric/odeint/stepper/generation.hpp>

using namespace boost::numeric::odeint;

template< class Stepper1 , class Stepper2 >
void check_stepper_type( const Stepper1 &s1 , const Stepper2 &s2 )
{
    BOOST_STATIC_ASSERT(( boost::is_same< Stepper1 , Stepper2 >::value ));
}

BOOST_AUTO_TEST_SUITE( generation_test )

typedef std::vector< double > state_type;

typedef runge_kutta_cash_karp54< state_type > rk54_type;
typedef runge_kutta_cash_karp54_classic< state_type > rk54_classic_type;
typedef runge_kutta_dopri5< state_type > dopri5_type;
typedef runge_kutta_fehlberg78< state_type > fehlberg78_type;
typedef rosenbrock4< double > rosenbrock4_type;


BOOST_AUTO_TEST_CASE( test_generation_dopri5 )
{
    check_stepper_type( make_controlled( 1.0e-6 , 1.0e-10 , dopri5_type() ) , controlled_runge_kutta< dopri5_type >() );
    check_stepper_type( make_controlled< dopri5_type >( 1.0e-6 , 1.0e-10 ) , controlled_runge_kutta< dopri5_type >() );

    check_stepper_type( make_dense_output( 1.0e-6 , 1.0e-10 , dopri5_type() ) , dense_output_runge_kutta< controlled_runge_kutta< dopri5_type > >() );
    check_stepper_type( make_dense_output< dopri5_type >( 1.0e-6 , 1.0e-10 ) , dense_output_runge_kutta< controlled_runge_kutta< dopri5_type > >() );
}

BOOST_AUTO_TEST_CASE( test_generation_cash_karp54 )
{
    check_stepper_type( make_controlled( 1.0e-6 , 1.0e-10 , rk54_type() ) , controlled_runge_kutta< rk54_type >() );
    check_stepper_type( make_controlled< rk54_type >( 1.0e-6 , 1.0e-10 ) , controlled_runge_kutta< rk54_type >() );
}

BOOST_AUTO_TEST_CASE( test_generation_cash_karp54_classic )
{
    check_stepper_type( make_controlled( 1.0e-6 , 1.0e-10 , rk54_classic_type() ) , controlled_runge_kutta< rk54_classic_type >() );
    check_stepper_type( make_controlled< rk54_classic_type >( 1.0e-6 , 1.0e-10 ) , controlled_runge_kutta< rk54_classic_type >() );
}


BOOST_AUTO_TEST_CASE( test_generation_fehlberg78 )
{
    check_stepper_type( make_controlled( 1.0e-6 , 1.0e-10 , fehlberg78_type() ) , controlled_runge_kutta< fehlberg78_type >() );
    check_stepper_type( make_controlled< fehlberg78_type >( 1.0e-6 , 1.0e-10 ) , controlled_runge_kutta< fehlberg78_type >() );
}


BOOST_AUTO_TEST_CASE( test_generation_rosenbrock4 )
{
    check_stepper_type( make_controlled( 1.0e-6 , 1.0e-10 , rosenbrock4_type() ) , rosenbrock4_controller< rosenbrock4_type >() );
    check_stepper_type( make_controlled< rosenbrock4_type >( 1.0e-6 , 1.0e-10 ) , rosenbrock4_controller< rosenbrock4_type >() );

    check_stepper_type( make_dense_output( 1.0e-6 , 1.0e-10 , rosenbrock4_type() ) , rosenbrock4_dense_output< rosenbrock4_controller< rosenbrock4_type > >() );
    check_stepper_type( make_dense_output< rosenbrock4_type >( 1.0e-6 , 1.0e-10 ) , rosenbrock4_dense_output< rosenbrock4_controller< rosenbrock4_type > >() );
}


BOOST_AUTO_TEST_SUITE_END()
