/*
 [auto_generated]
 libs/numeric/odeint/test/adams_bashforth_moulton.cpp

 [begin_description]
 This file tests the use of the Adams-Bashforth-Moulton.
 [end_description]

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#define BOOST_TEST_MODULE odeint_adams_bashforth_moulton

#include <utility>

#include <boost/test/unit_test.hpp>

#include <boost/mpl/list.hpp>
#include <boost/mpl/size_t.hpp>
#include <boost/mpl/range_c.hpp>

#include <boost/numeric/odeint/stepper/adams_bashforth_moulton.hpp>

using namespace boost::unit_test;
using namespace boost::numeric::odeint;

typedef double value_type;

struct lorenz
{
    template< class State , class Deriv , class Value >
    void operator()( const State &_x , Deriv &_dxdt , const Value &dt ) const
    {
        const value_type sigma = 10.0;
        const value_type R = 28.0;
        const value_type b = 8.0 / 3.0;

        typename boost::range_iterator< const State >::type x = boost::begin( _x );
        typename boost::range_iterator< Deriv >::type dxdt = boost::begin( _dxdt );

        dxdt[0] = sigma * ( x[1] - x[0] );
        dxdt[1] = R * x[0] - x[1] - x[0] * x[2];
        dxdt[2] = x[0]*x[1] - b * x[2];
    }
};

BOOST_AUTO_TEST_SUITE( adams_bashforth_moulton_test )

typedef boost::mpl::range_c< size_t , 1 , 6 > vector_of_steps;

BOOST_AUTO_TEST_CASE_TEMPLATE( test_init_and_steps , step_type , vector_of_steps )
{
    const static size_t steps = step_type::value;
    typedef boost::array< value_type , 3 > state_type;

    adams_bashforth_moulton< steps , state_type > stepper;
    state_type x = {{ 10.0 , 10.0 , 10.0 }};
    const value_type dt = 0.01;
    value_type t = 0.0;

    stepper.initialize( lorenz() , x , t , dt );
    BOOST_CHECK_CLOSE( t , value_type( steps - 1 ) * dt , 1.0e-14 );

    stepper.do_step( lorenz() , x , t , dt );
}


BOOST_AUTO_TEST_CASE( test_copying )
{
    typedef boost::array< double , 1 > state_type;
    typedef adams_bashforth_moulton< 2 , state_type > stepper_type;

    stepper_type s1;

    stepper_type s2( s1 );

    stepper_type s3;
    s3 = s1;
 }


BOOST_AUTO_TEST_CASE( test_instantiation )
{
    typedef boost::array< double , 3 > state_type;
    adams_bashforth_moulton< 1 , state_type > s1;
    adams_bashforth_moulton< 2 , state_type > s2;
    adams_bashforth_moulton< 3 , state_type > s3;
    adams_bashforth_moulton< 4 , state_type > s4;
    adams_bashforth_moulton< 5 , state_type > s5;
    adams_bashforth_moulton< 6 , state_type > s6;
    adams_bashforth_moulton< 7 , state_type > s7;
    adams_bashforth_moulton< 8 , state_type > s8;

    state_type x = {{ 10.0 , 10.0 , 10.0 }};
    value_type t = 0.0 , dt = 0.01;
    s1.do_step( lorenz() , x , t , dt );
    s2.do_step( lorenz() , x , t , dt );
    s3.do_step( lorenz() , x , t , dt );
    s4.do_step( lorenz() , x , t , dt );
    s5.do_step( lorenz() , x , t , dt );
    s6.do_step( lorenz() , x , t , dt );
//  s7.do_step( lorenz() , x , t , dt );
//  s8.do_step( lorenz() , x , t , dt );
}


BOOST_AUTO_TEST_SUITE_END()
