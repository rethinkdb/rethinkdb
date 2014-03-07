/*
 [auto_generated]
 libs/numeric/odeint/test/adams_bashforth.cpp

 [begin_description]
 This file tests the use of the adams bashforth stepper.
 [end_description]

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


// disable checked iterator warning for msvc

#include <boost/config.hpp>
#ifdef BOOST_MSVC
    #pragma warning(disable:4996)
#endif

#define BOOST_TEST_MODULE odeint_adams_bashforth

#include <utility>

#include <boost/array.hpp>

#include <boost/test/unit_test.hpp>

#include <boost/mpl/list.hpp>
#include <boost/mpl/size_t.hpp>
#include <boost/mpl/range_c.hpp>


#include <boost/numeric/odeint/stepper/adams_bashforth.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta4.hpp>

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

template< class State >
class rk4_decorator
{
public:

    size_t do_count;

    template< class System , class StateIn , class DerivIn , class StateOut >
    void do_step( System system , const StateIn &in , const DerivIn &dxdt , value_type t , StateOut &out , value_type dt )
    {
        m_stepper.do_step( system , in , dxdt , t , out , dt );
        ++do_count;
    }

    template< class System , class StateInOut , class DerivIn >
    void do_step( System system , StateInOut &x , const DerivIn &dxdt , value_type t , value_type dt )
    {
        m_stepper.do_step( system , x , dxdt , t , dt );
        ++do_count;
    }


    runge_kutta4< State > m_stepper;

private:


};


BOOST_AUTO_TEST_SUITE( adams_bashforth_test )

BOOST_AUTO_TEST_CASE( test_adams_bashforth_coefficients )
{
    detail::adams_bashforth_coefficients< value_type , 1 > c1;
    detail::adams_bashforth_coefficients< value_type , 2 > c2;
    detail::adams_bashforth_coefficients< value_type , 3 > c3;
    detail::adams_bashforth_coefficients< value_type , 4 > c4;
    detail::adams_bashforth_coefficients< value_type , 5 > c5;
    detail::adams_bashforth_coefficients< value_type , 6 > c6;
    detail::adams_bashforth_coefficients< value_type , 7 > c7;
    detail::adams_bashforth_coefficients< value_type , 8 > c8;
}

BOOST_AUTO_TEST_CASE( test_rotating_buffer )
{
    const size_t N = 5;
    detail::rotating_buffer< size_t , N > buffer;
    for( size_t i=0 ; i<N ; ++i ) buffer[i] = i;

    for( size_t i=0 ; i<N ; ++i )
        BOOST_CHECK_EQUAL( buffer[i] , i );

    buffer.rotate();

    for( size_t i=1 ; i<N ; ++i )
        BOOST_CHECK_EQUAL( buffer[i] , i - 1 );
    BOOST_CHECK_EQUAL( buffer[0] , size_t( N-1 ) );
}

BOOST_AUTO_TEST_CASE( test_copying )
{
    typedef boost::array< double , 1 > state_type;
    typedef adams_bashforth< 2 , state_type > stepper_type;

    stepper_type s1;
    s1.step_storage()[0].m_v[0] = 1.5;
    s1.step_storage()[1].m_v[0] = 2.25;

    stepper_type s2( s1 );
    BOOST_CHECK_CLOSE( s1.step_storage()[0].m_v[0] , s2.step_storage()[0].m_v[0] , 1.0e-14 );
    BOOST_CHECK_CLOSE( s1.step_storage()[1].m_v[0] , s2.step_storage()[1].m_v[0] , 1.0e-14 );
    BOOST_CHECK( ( &(s1.step_storage()[0]) ) != ( &(s2.step_storage()[0]) ) );

    stepper_type s3;
    state_type *p1 = &( s3.step_storage()[0].m_v ) , *p2 = &( s3.step_storage()[1].m_v );
    s3 = s1;
    BOOST_CHECK( p1 == ( &( s3.step_storage()[0].m_v ) ) );
    BOOST_CHECK( p2 == ( &( s3.step_storage()[1].m_v ) ) );

    BOOST_CHECK_CLOSE( s1.step_storage()[0].m_v[0] , s3.step_storage()[0].m_v[0] , 1.0e-14 );
    BOOST_CHECK_CLOSE( s1.step_storage()[1].m_v[0] , s3.step_storage()[1].m_v[0] , 1.0e-14 );
}

typedef boost::mpl::range_c< size_t , 1 , 6 > vector_of_steps;

BOOST_AUTO_TEST_CASE_TEMPLATE( test_init_and_steps , step_type , vector_of_steps )
{
    const static size_t steps = step_type::value;
    typedef boost::array< value_type , 3 > state_type;

    adams_bashforth< steps , state_type > stepper;
    state_type x = {{ 10.0 , 10.0 , 10.0 }};
    const value_type dt = 0.01;
    value_type t = 0.0;

    stepper.initialize( lorenz() , x , t , dt );
    BOOST_CHECK_CLOSE( t , value_type( steps - 1 ) * dt , 1.0e-14 );

    stepper.do_step( lorenz() , x , t , dt );
}

BOOST_AUTO_TEST_CASE( test_instantiation )
{
    typedef boost::array< double , 3 > state_type;
    adams_bashforth< 1 , state_type > s1;
    adams_bashforth< 2 , state_type > s2;
    adams_bashforth< 3 , state_type > s3;
    adams_bashforth< 4 , state_type > s4;
    adams_bashforth< 5 , state_type > s5;
    adams_bashforth< 6 , state_type > s6;
    adams_bashforth< 7 , state_type > s7;
    adams_bashforth< 8 , state_type > s8;

    state_type x = {{ 10.0 , 10.0 , 10.0 }};
    value_type t = 0.0 , dt = 0.01;
    s1.do_step( lorenz() , x , t , dt );
    s2.do_step( lorenz() , x , t , dt );
    s3.do_step( lorenz() , x , t , dt );
    s4.do_step( lorenz() , x , t , dt );
    s5.do_step( lorenz() , x , t , dt );
    s6.do_step( lorenz() , x , t , dt );
//    s7.do_step( lorenz() , x , t , dt );
//    s8.do_step( lorenz() , x , t , dt );
}

BOOST_AUTO_TEST_CASE( test_auto_initialization )
{
    typedef boost::array< double , 3 > state_type;
    state_type x = {{ 10.0 , 10.0 , 10.0 }};

    adams_bashforth< 3 , state_type , value_type , state_type , value_type , range_algebra , default_operations ,
                     initially_resizer , rk4_decorator< state_type > > adams;

    adams.initializing_stepper().do_count = 0;
    adams.do_step( lorenz() , x , 0.0 , x , 0.1 );
    BOOST_CHECK_EQUAL( adams.initializing_stepper().do_count , size_t( 1 ) );

    adams.do_step( lorenz() , x , 0.0 , x , 0.1 );
    BOOST_CHECK_EQUAL( adams.initializing_stepper().do_count , size_t( 2 ) );

    adams.do_step( lorenz() , x , 0.0 , x , 0.1 );
    BOOST_CHECK_EQUAL( adams.initializing_stepper().do_count , size_t( 2 ) );

    adams.do_step( lorenz() , x , 0.0 , x , 0.1 );
    BOOST_CHECK_EQUAL( adams.initializing_stepper().do_count , size_t( 2 ) );

    adams.do_step( lorenz() , x , 0.0 , x , 0.1 );
    BOOST_CHECK_EQUAL( adams.initializing_stepper().do_count , size_t( 2 ) );
}

BOOST_AUTO_TEST_CASE( test_manual_initialization )
{
    typedef boost::array< double , 3 > state_type;
    state_type x = {{ 10.0 , 10.0 , 10.0 }};

    adams_bashforth< 3 , state_type , value_type , state_type , value_type , range_algebra , default_operations ,
        initially_resizer , rk4_decorator< state_type > > adams;

    adams.initializing_stepper().do_count = 0;
    double t = 0.0 , dt = 0.1;
    adams.initialize( lorenz() , x , t , dt );
    BOOST_CHECK_EQUAL( adams.initializing_stepper().do_count , size_t( 2 ) );

    adams.do_step( lorenz() , x , 0.0 , x , 0.1 );
    BOOST_CHECK_EQUAL( adams.initializing_stepper().do_count , size_t( 2 ) );
}

BOOST_AUTO_TEST_SUITE_END()
