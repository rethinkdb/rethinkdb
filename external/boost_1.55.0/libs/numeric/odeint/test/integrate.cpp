/*
 [auto_generated]
 libs/numeric/odeint/test/integrate.cpp

 [begin_description]
 This file tests the integrate function and its variants.
 [end_description]

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#define BOOST_TEST_MODULE odeint_integrate_functions

#include <vector>
#include <cmath>
#include <iostream>

#include <boost/numeric/odeint/config.hpp>

#include <boost/array.hpp>
#include <boost/ref.hpp>
#include <boost/iterator/counting_iterator.hpp>

#include <boost/test/unit_test.hpp>

#include <boost/mpl/vector.hpp>

// nearly everything from odeint is used in these tests
#include <boost/numeric/odeint/integrate/integrate_const.hpp>
#include <boost/numeric/odeint/integrate/integrate_adaptive.hpp>
#include <boost/numeric/odeint/integrate/integrate_times.hpp>
#include <boost/numeric/odeint/integrate/integrate_n_steps.hpp>
#include <boost/numeric/odeint/stepper/euler.hpp>
#include <boost/numeric/odeint/stepper/modified_midpoint.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta4.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta_cash_karp54.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta_dopri5.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta_fehlberg78.hpp>
#include <boost/numeric/odeint/stepper/controlled_runge_kutta.hpp>
#include <boost/numeric/odeint/stepper/bulirsch_stoer.hpp>
#include <boost/numeric/odeint/stepper/dense_output_runge_kutta.hpp>
#include <boost/numeric/odeint/stepper/bulirsch_stoer_dense_out.hpp>

#include <boost/numeric/odeint/util/detail/less_with_sign.hpp>

using namespace boost::unit_test;
using namespace boost::numeric::odeint;
namespace mpl = boost::mpl;


typedef double value_type;
typedef std::vector< value_type > state_type;

void lorenz( const state_type &x , state_type &dxdt , const value_type t )
{
    //const value_type sigma( 10.0 );
    const value_type R( 28.0 );
    const value_type b( value_type( 8.0 ) / value_type( 3.0 ) );

    // first component trivial
    dxdt[0] = 1.0; //sigma * ( x[1] - x[0] );
    dxdt[1] = R * x[0] - x[1] - x[0] * x[2];
    dxdt[2] = -b * x[2] + x[0] * x[1];
}

struct push_back_time
{
    std::vector< double >& m_times;

    state_type& m_x;

    push_back_time( std::vector< double > &times , state_type &x )
    :  m_times( times ) , m_x( x ) { }

    void operator()( const state_type &x , double t )
    {
        m_times.push_back( t );
        boost::numeric::odeint::copy( x , m_x );
    }
};

template< class Stepper >
struct perform_integrate_const_test
{
    void operator()( const value_type t_end , const value_type dt )
    {
        std::cout << "Testing integrate_const with " << typeid( Stepper ).name() << std::endl;

        state_type x( 3 , 10.0 ) , x_end( 3 );

        std::vector< value_type > times;

        integrate_const( Stepper() , lorenz , x , 0.0 , t_end ,
                                        dt , push_back_time( times , x_end ) );

        // int steps = times.size()-1;

        //std::cout << t_end << " (" << dt << "), " << steps << " , " << times.size() << " , " << 10.0+dt*steps << "=" << x_end[0] << std::endl;

        BOOST_CHECK_EQUAL( static_cast<int>(times.size()) , static_cast<int>(floor(t_end/dt))+1 );

        for( size_t i=0 ; i<times.size() ; ++i )
        {
            //std::cout << i << " , " << times[i] << " , " << static_cast< value_type >(i)*dt << std::endl;
            // check if observer was called at times 0,1,2,...
            BOOST_CHECK_SMALL( times[i] - static_cast< value_type >(i)*dt , (i+1) * 2E-16 );
        }

        // check first, trivial, component
        BOOST_CHECK_SMALL( (10.0 + times[times.size()-1]) - x_end[0] , 1E-6 ); // precision of steppers: 1E-6
        //BOOST_CHECK_EQUAL( x[1] , x_end[1] );
        //BOOST_CHECK_EQUAL( x[2] , x_end[2] );
    }
};

template< class Stepper >
struct perform_integrate_adaptive_test
{
    void operator()( const value_type t_end = 10.0 , const value_type dt = 0.03 )
    {
        std::cout << "Testing integrate_adaptive with " << typeid( Stepper ).name() << std::endl;

        state_type x( 3 , 10.0 ) , x_end( 3 );

        std::vector< value_type > times;

        size_t steps = integrate_adaptive( Stepper() , *lorenz , x , 0.0 , t_end ,
                                        dt , push_back_time( times , x_end ) );

//        std::cout << t_end << " , " << steps << " , " << times.size() << " , " << 10.0+dt*steps << "=" << x_end[0] << std::endl;

        BOOST_CHECK_EQUAL( times.size() , steps+1 );

        BOOST_CHECK_SMALL( times[0] - 0.0 , 2E-16 );
        BOOST_CHECK_SMALL( times[times.size()-1] - t_end , times.size() * 2E-16 );

        // check first, trivial, component
        BOOST_CHECK_SMALL( (10.0 + t_end) - x_end[0] , 1E-6 ); // precision of steppers: 1E-6
//        BOOST_CHECK_EQUAL( x[1] , x_end[1] );
//        BOOST_CHECK_EQUAL( x[2] , x_end[2] );
    }
};


template< class Stepper >
struct perform_integrate_times_test
{
    void operator()( const int n = 10 , const int dn=1 , const value_type dt = 0.03 )
    {
        std::cout << "Testing integrate_times with " << typeid( Stepper ).name() << std::endl;

        state_type x( 3 ) , x_end( 3 );
        x[0] = x[1] = x[2] = 10.0;

        std::vector< double > times;

        std::vector< double > obs_times( abs(n) );
        for( int i=0 ; boost::numeric::odeint::detail::less_with_sign( i ,
                       static_cast<int>(obs_times.size()) ,
                       dt ) ; i+=dn )
        {
            obs_times[i] = i;
        }
        // simple stepper
        integrate_times( Stepper() , lorenz , x , obs_times.begin() , obs_times.end() ,
                    dt , push_back_time( times , x_end ) );

        BOOST_CHECK_EQUAL( static_cast<int>(times.size()) , abs(n) );

        for( size_t i=0 ; i<times.size() ; ++i )
            // check if observer was called at times 0,1,2,...
            BOOST_CHECK_EQUAL( times[i] , static_cast<double>(i) );

        // check first, trivial, component
        BOOST_CHECK_SMALL( (10.0 + 1.0*times[times.size()-1]) - x_end[0] , 1E-6 ); // precision of steppers: 1E-6
//        BOOST_CHECK_EQUAL( x[1] , x_end[1] );
//        BOOST_CHECK_EQUAL( x[2] , x_end[2] );
    }
};

template< class Stepper >
struct perform_integrate_n_steps_test
{
    void operator()( const int n = 200 , const value_type dt = 0.01 )
    {
        std::cout << "Testing integrate_n_steps with " << typeid( Stepper ).name() << std::endl;

        state_type x( 3 ) , x_end( 3 );
        x[0] = x[1] = x[2] = 10.0;

        std::vector< double > times;

        // simple stepper
        value_type end_time = integrate_n_steps( Stepper() , lorenz , x , 0.0 , dt , n , push_back_time( times , x_end ) );

        BOOST_CHECK_SMALL( end_time - n*dt , 2E-16 );
        BOOST_CHECK_EQUAL( static_cast<int>(times.size()) , n+1 );

        for( size_t i=0 ; i<times.size() ; ++i )
            // check if observer was called at times 0,1,2,...
            BOOST_CHECK_SMALL( times[i] - static_cast< value_type >(i)*dt , 2E-16 );

        // check first, trivial, component
        BOOST_CHECK_SMALL( (10.0 + end_time) - x_end[0] , 1E-6 ); // precision of steppers: 1E-6
//        BOOST_CHECK_EQUAL( x[1] , x_end[1] );
//        BOOST_CHECK_EQUAL( x[2] , x_end[2] );

    }
};



class stepper_methods : public mpl::vector<
    euler< state_type > ,
    modified_midpoint< state_type > ,
    runge_kutta4< state_type > ,
    runge_kutta_cash_karp54< state_type > ,
    runge_kutta_dopri5< state_type > ,
    runge_kutta_fehlberg78< state_type > ,
    controlled_runge_kutta< runge_kutta_cash_karp54< state_type > > ,
    controlled_runge_kutta< runge_kutta_dopri5< state_type > > ,
    controlled_runge_kutta< runge_kutta_fehlberg78< state_type > > ,
    bulirsch_stoer< state_type > ,
    dense_output_runge_kutta< controlled_runge_kutta< runge_kutta_dopri5< state_type > > >
    //bulirsch_stoer_dense_out< state_type >
> { };



BOOST_AUTO_TEST_SUITE( integrate_test )

BOOST_AUTO_TEST_CASE_TEMPLATE( integrate_const_test_case , Stepper, stepper_methods )
{
    perform_integrate_const_test< Stepper > tester;
    tester( 1.005 , 0.01 );
    tester( 1.0 , 0.01 );
    tester( 1.1 , 0.01 );
    tester( -1.005 , -0.01 );
}


BOOST_AUTO_TEST_CASE_TEMPLATE( integrate_adaptive_test_case , Stepper, stepper_methods )
{
    perform_integrate_adaptive_test< Stepper > tester;
    tester( 1.005 , 0.01 );
    tester( 1.0 , 0.01 );
    tester( 1.1 , 0.01 );
    tester( -1.005 , -0.01 );
}


BOOST_AUTO_TEST_CASE_TEMPLATE( integrate_times_test_case , Stepper, stepper_methods )
{
    perform_integrate_times_test< Stepper > tester;
    tester();
    //tester( -10 , -0.01 );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( integrate_n_steps_test_case , Stepper, stepper_methods )
{
    perform_integrate_n_steps_test< Stepper > tester;
    tester();
    tester( 200 , 0.01 );
    tester( 200 , 0.01 );
    tester( 200 , 0.01 );
    tester( 200 , -0.01 );
}

BOOST_AUTO_TEST_SUITE_END()
