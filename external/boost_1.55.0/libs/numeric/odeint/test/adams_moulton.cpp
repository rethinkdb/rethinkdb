/*
 [auto_generated]
 libs/numeric/odeint/test/adams_moulton.cpp

 [begin_description]
 This file tests the use of the Adams-Moulton stepper.
 [end_description]

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#define BOOST_TEST_MODULE odeint_adams_moulton

#include <utility>

#include <boost/array.hpp>

#include <boost/test/unit_test.hpp>

#include <boost/mpl/list.hpp>
#include <boost/mpl/size_t.hpp>
#include <boost/mpl/range_c.hpp>


#include <boost/numeric/odeint/stepper/detail/adams_moulton_coefficients.hpp>
#include <boost/numeric/odeint/stepper/detail/rotating_buffer.hpp>
#include <boost/numeric/odeint/stepper/adams_moulton.hpp>

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

BOOST_AUTO_TEST_SUITE( adams_moulton_test )

BOOST_AUTO_TEST_CASE( test_adams_moulton_coefficients )
{
    detail::adams_moulton_coefficients< value_type , 1 > c1;
    detail::adams_moulton_coefficients< value_type , 2 > c2;
    detail::adams_moulton_coefficients< value_type , 3 > c3;
    detail::adams_moulton_coefficients< value_type , 4 > c4;
    detail::adams_moulton_coefficients< value_type , 5 > c5;
    detail::adams_moulton_coefficients< value_type , 6 > c6;
    detail::adams_moulton_coefficients< value_type , 7 > c7;
    detail::adams_moulton_coefficients< value_type , 8 > c8;
}

typedef boost::mpl::range_c< size_t , 1 , 6 > vector_of_steps;

BOOST_AUTO_TEST_CASE_TEMPLATE( test_init_and_steps , step_type , vector_of_steps )
{
    const static size_t steps = step_type::value;
    typedef boost::array< value_type , 3 > state_type;

    adams_moulton< steps , state_type > stepper;
//    state_type x = {{ 10.0 , 10.0 , 10.0 }};
//    const value_type dt = 0.01;
//    value_type t = 0.0;

//    stepper.do_step( lorenz() , x , t , dt );
}

BOOST_AUTO_TEST_CASE( test_instantiation )
{
    typedef boost::array< double , 3 > state_type;
    adams_moulton< 1 , state_type > s1;
    adams_moulton< 2 , state_type > s2;
    adams_moulton< 3 , state_type > s3;
    adams_moulton< 4 , state_type > s4;
    adams_moulton< 5 , state_type > s5;
    adams_moulton< 6 , state_type > s6;
    adams_moulton< 7 , state_type > s7;
    adams_moulton< 8 , state_type > s8;

//    state_type x = {{ 10.0 , 10.0 , 10.0 }};
//    value_type t = 0.0 , dt = 0.01;
}


BOOST_AUTO_TEST_SUITE_END()
