/*
 [auto_generated]
 libs/numeric/odeint/test/integrate_implicit.cpp

 [begin_description]
 This file tests the integrate function and its variants with the implicit steppers.
 [end_description]

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#define BOOST_TEST_MODULE odeint_integrate_functions_implicit

#include <vector>
#include <cmath>
#include <iostream>

#include <boost/numeric/odeint/config.hpp>

#include <boost/array.hpp>
#include <boost/ref.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>

#include <boost/test/unit_test.hpp>

#include <boost/mpl/vector.hpp>

#include <boost/numeric/odeint/integrate/integrate_const.hpp>
#include <boost/numeric/odeint/integrate/integrate_adaptive.hpp>
#include <boost/numeric/odeint/integrate/integrate_times.hpp>
#include <boost/numeric/odeint/integrate/integrate_n_steps.hpp>
#include <boost/numeric/odeint/stepper/rosenbrock4.hpp>
#include <boost/numeric/odeint/stepper/rosenbrock4_controller.hpp>
#include <boost/numeric/odeint/stepper/rosenbrock4_dense_output.hpp>


using namespace boost::unit_test;
using namespace boost::numeric::odeint;
namespace mpl = boost::mpl;
namespace ublas = boost::numeric::ublas;

typedef double value_type;
typedef ublas::vector< value_type > state_type;
typedef boost::numeric::ublas::matrix< value_type > matrix_type;

struct sys
{
    void operator()( const state_type &x , state_type &dxdt , const value_type &t ) const
    {
        dxdt( 0 ) = x( 0 ) + 2 * x( 1 );
        dxdt( 1 ) = x( 1 );
    }
};

struct jacobi
{
    void operator()( const state_type &x , matrix_type &jacobi , const value_type &t , state_type &dfdt ) const
    {
        jacobi( 0 , 0 ) = 1;
        jacobi( 0 , 1 ) = 2;
        jacobi( 1 , 0 ) = 0;
        jacobi( 1 , 1 ) = 1;
        dfdt( 0 ) = 0.0;
        dfdt( 1 ) = 0.0;
    }
};

struct push_back_time
{
    std::vector< double >& m_times;

    push_back_time( std::vector< double > &times )
    :  m_times( times ) { }

    void operator()( const state_type &x , double t )
    {
        m_times.push_back( t );
    }
};

template< class Stepper >
struct perform_integrate_const_test
{
    void operator()( void )
    {
        state_type x( 2 , 1.0 );
        const value_type dt = 0.03;
        const value_type t_end = 10.0;

        std::vector< value_type > times;

        integrate_const( Stepper() , std::make_pair( sys() , jacobi() ) , x , 0.0 , t_end ,
                                        dt , push_back_time( times ) );

        BOOST_CHECK_EQUAL( static_cast<int>(times.size()) , static_cast<int>(floor(t_end/dt))+1 );

        for( size_t i=0 ; i<times.size() ; ++i )
        {
            //std::cout << i << std::endl;
            // check if observer was called at times 0,1,2,...
            BOOST_CHECK_SMALL( times[i] - static_cast< value_type >(i)*dt , (i+1) * 2E-16 );
        }
    }
};

template< class Stepper >
struct perform_integrate_adaptive_test
{
    void operator()( void )
    {
        state_type x( 2 , 1.0 );
        const value_type dt = 0.03;
        const value_type t_end = 10.0;

        std::vector< value_type > times;

        size_t steps = integrate_adaptive( Stepper() , std::make_pair( sys() , jacobi() ) , x , 0.0 , t_end ,
                                        dt , push_back_time( times ) );

        BOOST_CHECK_EQUAL( times.size() , steps+1 );

        BOOST_CHECK_SMALL( times[0] - 0.0 , 2E-16 );
        BOOST_CHECK_SMALL( times[times.size()-1] - t_end , times.size() * 2E-16 );
    }
};


template< class Stepper >
struct perform_integrate_times_test
{
    void operator()( void )
    {
        state_type x( 2 , 1.0 );

        const value_type dt = 0.03;

        std::vector< double > times;

        // simple stepper
        integrate_times( Stepper() , std::make_pair( sys() , jacobi() ) , x , 
                    boost::counting_iterator<int>(0) , boost::counting_iterator<int>(10) ,
                    dt , push_back_time( times ) );

        BOOST_CHECK_EQUAL( static_cast<int>(times.size()) , 10 );

        for( size_t i=0 ; i<times.size() ; ++i )
            // check if observer was called at times 0,1,2,...
            BOOST_CHECK_EQUAL( times[i] , static_cast<double>(i) );
    }
};

template< class Stepper >
struct perform_integrate_n_steps_test
{
    void operator()( void )
    {
        state_type x( 2 , 1.0 );

        const value_type dt = 0.03;
        const int n = 200;

        std::vector< double > times;

        // simple stepper
        value_type end_time = integrate_n_steps( Stepper() , std::make_pair( sys() , jacobi() ) , x , 0.0 , dt , n , push_back_time( times ) );

        BOOST_CHECK_SMALL( end_time - n*dt , 2E-16 );
        BOOST_CHECK_EQUAL( static_cast<int>(times.size()) , n+1 );

        for( size_t i=0 ; i<times.size() ; ++i )
            // check if observer was called at times 0,1,2,...
            BOOST_CHECK_SMALL( times[i] - static_cast< value_type >(i)*dt , (i+1) * 2E-16 );
    }
};



class stepper_methods : public mpl::vector<
    rosenbrock4< value_type > ,
    rosenbrock4_controller< rosenbrock4< value_type > > ,
    rosenbrock4_dense_output< rosenbrock4_controller< rosenbrock4< value_type > > >
> { };



BOOST_AUTO_TEST_SUITE( integrate_test )

BOOST_AUTO_TEST_CASE_TEMPLATE( integrate_const_test_case , Stepper, stepper_methods )
{
    perform_integrate_const_test< Stepper > tester;
    tester();
}


BOOST_AUTO_TEST_CASE_TEMPLATE( integrate_adaptive_test_case , Stepper, stepper_methods )
{
    perform_integrate_adaptive_test< Stepper > tester;
    tester();
}


BOOST_AUTO_TEST_CASE_TEMPLATE( integrate_times_test_case , Stepper, stepper_methods )
{
    perform_integrate_times_test< Stepper > tester;
    tester();
}


class simple_stepper_methods : public mpl::vector<
    rosenbrock4< value_type >
> { };

BOOST_AUTO_TEST_CASE_TEMPLATE( integrate_n_steps_test_case , Stepper, simple_stepper_methods )
{
    perform_integrate_n_steps_test< Stepper > tester;
    tester();
}

BOOST_AUTO_TEST_SUITE_END()
