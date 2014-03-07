/*
 [auto_generated]
 libs/numeric/odeint/test/runge_kutta_concepts.cpp

 [begin_description]
 This file tests the Stepper concepts of odeint with all Runge-Kutta steppers. It's one of the main tests
 of odeint.
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

#define BOOST_TEST_MODULE odeint_runge_kutta_concepts

#include <vector>
#include <cmath>
#include <iostream>

#include <boost/numeric/odeint/config.hpp>

#include <boost/array.hpp>

#include <boost/test/unit_test.hpp>

#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/utility.hpp>
#include <boost/type_traits/add_reference.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/insert_range.hpp>
#include <boost/mpl/end.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/inserter.hpp>

#include <boost/numeric/odeint/stepper/euler.hpp>
#include <boost/numeric/odeint/stepper/modified_midpoint.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta4_classic.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta4.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta_cash_karp54_classic.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta_cash_karp54.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta_dopri5.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta_fehlberg78.hpp>

#include "prepare_stepper_testing.hpp"
#include "dummy_odes.hpp"

using std::vector;

using namespace boost::unit_test;
using namespace boost::numeric::odeint;
namespace mpl = boost::mpl;

const double result = 2.2;

const double eps = 1.0e-14;

template< class Stepper , class System >
void check_stepper_concept( Stepper &stepper , System system , typename Stepper::deriv_type &x )
{
    typedef Stepper stepper_type;
    typedef typename stepper_type::deriv_type container_type;
    typedef typename stepper_type::order_type order_type;
    typedef typename stepper_type::time_type time_type;

    stepper.do_step( system , x , 0.0 , 0.1 );
}


template< class Stepper , class State > struct perform_stepper_test;

template< class Stepper >
struct perform_stepper_test< Stepper , vector_type >
{
    void operator()( void )
    {
        vector_type x( 1 , 2.0 );
        Stepper stepper;
        check_stepper_concept( stepper , constant_system_standard< vector_type , vector_type , double > , x );
        check_stepper_concept( stepper , boost::cref( constant_system_functor_standard() ) , x );
        std::cout << x[0] << " ?= " << result << std::endl;
        BOOST_CHECK_SMALL( fabs( x[0] - result ) , eps );
    }
};

template< class Stepper >
struct perform_stepper_test< Stepper , vector_space_type >
{
    void operator()( void ) const
    {
        vector_space_type x;
        x.m_x = 2.0;
        Stepper stepper;
        check_stepper_concept( stepper , constant_system_vector_space< vector_space_type , vector_space_type , double > , x );
        check_stepper_concept( stepper , boost::cref( constant_system_functor_vector_space() ) , x );
        std::cout << x.m_x << " ?= " << result << std::endl;
        BOOST_CHECK_SMALL( fabs( x.m_x - result ) , eps );
    }
};

template< class Stepper >
struct perform_stepper_test< Stepper , array_type >
{
    void operator()( void )
    {
        array_type x;
        x[0] = 2.0;
        Stepper stepper;
        check_stepper_concept( stepper , constant_system_standard< array_type , array_type , double> , x );
        check_stepper_concept( stepper , boost::cref( constant_system_functor_standard() ) , x );
        std::cout << x[0] << " ?= " << result << std::endl;
        BOOST_CHECK_SMALL( fabs( x[0] - result ) , eps );
    }
};


template< class State > class stepper_methods : public mpl::vector<
    euler< State , double , State , double , typename algebra_dispatcher< State >::type > ,
    modified_midpoint< State , double , State , double , typename algebra_dispatcher< State >::type > ,
    runge_kutta4< State , double , State , double , typename algebra_dispatcher< State >::type > ,
    runge_kutta4_classic< State , double , State , double , typename algebra_dispatcher< State >::type > ,
    runge_kutta_cash_karp54_classic< State , double , State , double , typename algebra_dispatcher< State >::type > ,
    runge_kutta_cash_karp54< State , double , State , double , typename algebra_dispatcher< State >::type > ,
    runge_kutta_dopri5< State , double , State , double , typename algebra_dispatcher< State >::type > ,
    runge_kutta_fehlberg78< State , double , State , double , typename algebra_dispatcher< State >::type >
    > { };



typedef mpl::copy
<
  container_types ,
  mpl::inserter
  <
    mpl::vector0<> ,
    mpl::insert_range
    <
      mpl::_1 ,
      mpl::end< mpl::_1 > ,
      stepper_methods< mpl::_2 >
    >
  >
>::type all_stepper_methods;



BOOST_AUTO_TEST_SUITE( runge_kutta_concept_test )

BOOST_AUTO_TEST_CASE_TEMPLATE( stepper_test , Stepper, all_stepper_methods )
{
    perform_stepper_test< Stepper , typename Stepper::deriv_type > tester;
    tester();
}


BOOST_AUTO_TEST_SUITE_END()
