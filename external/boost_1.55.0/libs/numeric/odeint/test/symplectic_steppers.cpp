/*
 [auto_generated]
 libs/numeric/odeint/test/symplectic_steppers.cpp

 [begin_description]
 tba.
 [end_description]

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/config.hpp>
#ifdef BOOST_MSVC
    #pragma warning(disable:4996)
#endif

#define BOOST_TEST_MODULE odeint_symplectic_steppers

#define BOOST_FUSION_INVOKE_MAX_ARITY 15
#define BOOST_RESULT_OF_NUM_ARGS 15
#define FUSION_MAX_VECTOR_SIZE 15


#include <boost/test/unit_test.hpp>

#include <boost/array.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/zip_view.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/insert_range.hpp>
#include <boost/mpl/end.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/inserter.hpp>
#include <boost/mpl/at.hpp>

#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/include/make_vector.hpp>

#include <boost/numeric/odeint/algebra/vector_space_algebra.hpp>
#include <boost/numeric/odeint/algebra/fusion_algebra.hpp>
#include <boost/numeric/odeint/stepper/symplectic_euler.hpp>
#include <boost/numeric/odeint/stepper/symplectic_rkn_sb3a_mclachlan.hpp>
#include <boost/numeric/odeint/stepper/symplectic_rkn_sb3a_m4_mclachlan.hpp>

#include "diagnostic_state_type.hpp"
#include "const_range.hpp"
#include "dummy_odes.hpp"
#include "vector_space_1d.hpp"
#include "boost_units_helpers.hpp"


using namespace boost::unit_test;
using namespace boost::numeric::odeint;
namespace mpl = boost::mpl;
namespace fusion = boost::fusion;

class custom_range_algebra : public range_algebra { };
class custom_default_operations : public default_operations { };


template< class Coor , class Mom , class Value , class CoorDeriv , class MomDeriv , class Time ,
          class Algebra , class Operations , class Resizer >
class complete_steppers : public mpl::vector<
    symplectic_euler< Coor , Mom , Value , CoorDeriv , MomDeriv , Time ,
                      Algebra , Operations , Resizer >
    , symplectic_rkn_sb3a_mclachlan< Coor , Mom , Value , CoorDeriv , MomDeriv , Time ,
                                     Algebra , Operations , Resizer >
    , symplectic_rkn_sb3a_m4_mclachlan< Coor , Mom , Value , CoorDeriv , MomDeriv , Time ,
                                        Algebra , Operations , Resizer >
    > {};

template< class Resizer >
class vector_steppers : public complete_steppers<
    diagnostic_state_type , diagnostic_state_type2 , double ,
    diagnostic_deriv_type , diagnostic_deriv_type2 , double ,
    custom_range_algebra , custom_default_operations , Resizer
    > { };


typedef mpl::vector< initially_resizer , always_resizer , never_resizer > resizers;
typedef mpl::vector_c< int , 1 , 3 , 0 > resizer_multiplicities ;


typedef mpl::copy<
    resizers ,
    mpl::inserter<
        mpl::vector0<> ,
        mpl::insert_range<
            mpl::_1 ,
            mpl::end< mpl::_1 > ,
            vector_steppers< mpl::_2 >
            >
        >
    >::type all_stepper_methods;


typedef mpl::size< vector_steppers< initially_resizer > >::type num_steppers;
typedef mpl::copy<
    resizer_multiplicities ,
    mpl::inserter<
        mpl::vector0<> ,
        mpl::insert_range<
            mpl::_1 , 
            mpl::end< mpl::_1 > ,
            const_range< num_steppers , mpl::_2 >
            >
        >
    >::type all_multiplicities;
                                                                                 






BOOST_AUTO_TEST_SUITE( symplectic_steppers_test )


BOOST_AUTO_TEST_CASE_TEMPLATE( test_assoc_types , Stepper , vector_steppers< initially_resizer > )
{
    BOOST_STATIC_ASSERT_MSG(
        ( boost::is_same< typename Stepper::coor_type , diagnostic_state_type >::value ) ,
        "Coordinate type" );
    BOOST_STATIC_ASSERT_MSG(
        ( boost::is_same< typename Stepper::momentum_type , diagnostic_state_type2 >::value ) ,
        "Momentum type" );
    BOOST_STATIC_ASSERT_MSG(
        ( boost::is_same< typename Stepper::coor_deriv_type , diagnostic_deriv_type >::value ) ,
        "Coordinate deriv type" );
    BOOST_STATIC_ASSERT_MSG(
        ( boost::is_same< typename Stepper::momentum_deriv_type , diagnostic_deriv_type2 >::value ) ,
        "Momentum deriv type" );

    BOOST_STATIC_ASSERT_MSG(
        ( boost::is_same< typename Stepper::state_type , std::pair< diagnostic_state_type , diagnostic_state_type2 > >::value ) ,
        "State type" );
    BOOST_STATIC_ASSERT_MSG(
        ( boost::is_same< typename Stepper::deriv_type , std::pair< diagnostic_deriv_type , diagnostic_deriv_type2 > >::value ) ,
        "Deriv type" );

    BOOST_STATIC_ASSERT_MSG( ( boost::is_same< typename Stepper::value_type , double >::value ) , "Value type" );
    BOOST_STATIC_ASSERT_MSG( ( boost::is_same< typename Stepper::time_type , double >::value ) , "Time type" );
    BOOST_STATIC_ASSERT_MSG( ( boost::is_same< typename Stepper::algebra_type , custom_range_algebra >::value ) , "Algebra type" );
    BOOST_STATIC_ASSERT_MSG( ( boost::is_same< typename Stepper::operations_type , custom_default_operations >::value ) , "Operations type" );

    BOOST_STATIC_ASSERT_MSG( ( boost::is_same< typename Stepper::resizer_type , initially_resizer >::value ) , "Resizer type" );
    BOOST_STATIC_ASSERT_MSG( ( boost::is_same< typename Stepper::stepper_category , stepper_tag >::value ) , "Stepper category" );
}


BOOST_AUTO_TEST_CASE_TEMPLATE( test_adjust_size , Stepper , vector_steppers< initially_resizer > )
{
    counter_state::init_counter();
    counter_deriv::init_counter();
    counter_state2::init_counter();
    counter_deriv2::init_counter();

    {
        Stepper stepper;
        diagnostic_state_type x;
        stepper.adjust_size( x );
    }

    TEST_COUNTERS( counter_state , 0 , 0 , 0 , 0 );
    TEST_COUNTERS( counter_state2 , 0 , 0 , 0 , 0 );
    TEST_COUNTERS( counter_deriv , 1 , 1 , 0 , 1 );
    TEST_COUNTERS( counter_deriv2 , 1 , 1 , 0 , 1 );
}


typedef mpl::zip_view< mpl::vector< all_stepper_methods , all_multiplicities > >::type zipped_steppers;
BOOST_AUTO_TEST_CASE_TEMPLATE( test_resizing , Stepper , zipped_steppers )
{
    typedef typename mpl::at_c< Stepper , 0 >::type stepper_type;
    const size_t multiplicity = mpl::at_c< Stepper , 1 >::type::value;

    counter_state::init_counter();
    counter_deriv::init_counter();
    counter_state2::init_counter();
    counter_deriv2::init_counter();

    {
        stepper_type stepper;
        std::pair< diagnostic_state_type , diagnostic_state_type2 > x;
        stepper.do_step( constant_mom_func() , x , 0.0 , 0.1 );
        stepper.do_step( constant_mom_func() , x , 0.0 , 0.1 );
        stepper.do_step( constant_mom_func() , x , 0.0 , 0.1 );
    }

    TEST_COUNTERS( counter_state , 0 , 0 , 0 , 0 );
    TEST_COUNTERS( counter_state2 , 0 , 0 , 0 , 0 );
    TEST_COUNTERS( counter_deriv , multiplicity , 1 , 0 , 1 );
    TEST_COUNTERS( counter_deriv2 , multiplicity , 1 , 0 , 1 );
}


BOOST_AUTO_TEST_CASE_TEMPLATE( test_copying1 , Stepper , vector_steppers< initially_resizer > )
{
    counter_state::init_counter();
    counter_deriv::init_counter();
    counter_state2::init_counter();
    counter_deriv2::init_counter();

    {
        Stepper stepper;
        Stepper stepper2( stepper );
    }

    TEST_COUNTERS( counter_state , 0 , 0 , 0 , 0 );
    TEST_COUNTERS( counter_state2 , 0 , 0 , 0 , 0 );
    TEST_COUNTERS( counter_deriv , 0 , 2 , 1 , 2 );
    TEST_COUNTERS( counter_deriv2 , 0 , 2 , 1 , 2 );
}


BOOST_AUTO_TEST_CASE_TEMPLATE( test_copying2 , Stepper , vector_steppers< initially_resizer > )
{
    counter_state::init_counter();
    counter_deriv::init_counter();
    counter_state2::init_counter();
    counter_deriv2::init_counter();

    {
        Stepper stepper;
        std::pair< diagnostic_state_type , diagnostic_state_type2 > x;
        stepper.do_step( constant_mom_func() , x , 0.0 , 0.1 );
        Stepper stepper2( stepper );
    }

    TEST_COUNTERS( counter_state , 0 , 0 , 0 , 0 );
    TEST_COUNTERS( counter_state2 , 0 , 0 , 0 , 0 );
    TEST_COUNTERS( counter_deriv , 1 , 2 , 1 , 2 );
    TEST_COUNTERS( counter_deriv2 , 1 , 2 , 1 , 2 );
}


BOOST_AUTO_TEST_CASE_TEMPLATE( test_do_step_v1 , Stepper , vector_steppers< initially_resizer > )
{
    Stepper s;
    std::pair< diagnostic_state_type , diagnostic_state_type2 > x1 , x2 , x3 , x4;
    x1.first[0] = 1.0;
    x1.second[0] = 2.0;
    x2 = x3 = x4 = x1;
    diagnostic_state_type x5_coor , x5_mom;
    x5_coor[0] = x1.first[0];
    x5_mom[0] = x1.second[0];

    s.do_step( constant_mom_func() , x1 , 0.0 , 0.1 );

    s.do_step( std::make_pair( default_coor_func() , constant_mom_func() ) , x2 , 0.0 , 0.1 );

    default_coor_func cf;
    constant_mom_func mf;
    s.do_step( std::make_pair( boost::ref( cf ) , boost::ref( mf ) ) , x3 , 0.0 , 0.1 );

    std::pair< default_coor_func , constant_mom_func > pf;
    s.do_step( boost::ref( pf ) , x4 , 0.0 , 0.1 );

    s.do_step( constant_mom_func() , std::make_pair( boost::ref( x5_coor ) , boost::ref( x5_mom ) ) , 0.0 , 0.1 );

    // checking for absolute values is not possible here, since the steppers are to different
    BOOST_CHECK_CLOSE( x1.first[0] , x2.first[0] , 1.0e-14 );
    BOOST_CHECK_CLOSE( x2.first[0] , x3.first[0] , 1.0e-14 );
    BOOST_CHECK_CLOSE( x3.first[0] , x4.first[0] , 1.0e-14 );
    BOOST_CHECK_CLOSE( x4.first[0] , x5_coor[0] , 1.0e-14 );

    BOOST_CHECK_CLOSE( x1.second[0] , x2.second[0] , 1.0e-14 );
    BOOST_CHECK_CLOSE( x2.second[0] , x3.second[0] , 1.0e-14 );
    BOOST_CHECK_CLOSE( x3.second[0] , x4.second[0] , 1.0e-14 );
    BOOST_CHECK_CLOSE( x4.second[0] , x5_mom[0] , 1.0e-14 );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( test_do_step_range , Stepper , vector_steppers< initially_resizer > )
{
    Stepper s;
    diagnostic_state_type q , p ;
    q[0] = 1.0;
    p[0] = 2.0;

    std::vector< double > x;
    x.push_back( 1.0 );
    x.push_back( 2.0 );
    s.do_step( constant_mom_func() ,
               std::make_pair( x.begin() , x.begin() + 1 ) ,
               std::make_pair( x.begin() + 1 , x.begin() + 2 ) ,
               0.0 , 0.1 );

    s.do_step( constant_mom_func() , q , p , 0.0 , 0.1 );

    BOOST_CHECK_CLOSE( q[0] , x[0] , 1.0e-14 );
    BOOST_CHECK_CLOSE( p[0] , x[1] , 1.0e-14 );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( test_do_step_v2 , Stepper , vector_steppers< initially_resizer > )
{
    Stepper s;
    diagnostic_state_type q , p ;
    q[0] = 1.0;
    p[0] = 2.0;
    diagnostic_state_type q2 = q , p2 = p;


    s.do_step( constant_mom_func() , q , p , 0.0 , 0.1 );
    s.do_step( constant_mom_func() , std::make_pair( boost::ref( q2 ) , boost::ref( p2 ) ) , 0.0 , 0.1 );
    
    BOOST_CHECK_CLOSE( q[0] , q2[0] , 1.0e-14 );
    BOOST_CHECK_CLOSE( p[0] , p2[0] , 1.0e-14 );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( test_do_step_v3 , Stepper , vector_steppers< initially_resizer > )
{
    Stepper s;
    std::pair< diagnostic_state_type , diagnostic_state_type2 > x_in , x_out;
    x_in.first[0] = 1.0;
    x_in.second[0] = 2.0;
    diagnostic_state_type q2 , p2;
    q2[0] = x_in.first[0];
    p2[0] = x_in.second[0];


    s.do_step( constant_mom_func() , x_in , 0.0 , x_out , 0.1 );
    s.do_step( constant_mom_func() , std::make_pair( boost::ref( q2 ) , boost::ref( p2 ) ) , 0.0 , 0.1 );
    
    BOOST_CHECK_CLOSE( x_in.first[0] , 1.0 , 1.0e-14 );
    BOOST_CHECK_CLOSE( x_in.second[0] , 2.0 , 1.0e-14 );
    BOOST_CHECK_CLOSE( x_out.first[0] , q2[0] , 1.0e-14 );
    BOOST_CHECK_CLOSE( x_out.second[0] , p2[0] , 1.0e-14 );
}


typedef vector_space_1d< double > vector_space;
typedef complete_steppers< vector_space , vector_space , double , 
                           vector_space , vector_space , double , 
                           vector_space_algebra , default_operations , initially_resizer > vector_space_steppers;
BOOST_AUTO_TEST_CASE_TEMPLATE( test_with_vector_space_algebra , Stepper , vector_space_steppers )
{
    Stepper s;
    std::pair< vector_space , vector_space > x;
    s.do_step( constant_mom_func_vector_space_1d() , x , 0.0 , 0.1 );

    s.do_step( std::make_pair( default_coor_func_vector_space_1d() , constant_mom_func_vector_space_1d() ) , x , 0.0 , 0.1 );
}


typedef boost::fusion::vector< length_type > coor_type;
typedef boost::fusion::vector< velocity_type > mom_type;
typedef boost::fusion::vector< acceleration_type > acc_type;
typedef complete_steppers< coor_type , mom_type , double , 
                           mom_type , acc_type , time_type, 
                           fusion_algebra , default_operations , initially_resizer > boost_unit_steppers;
BOOST_AUTO_TEST_CASE_TEMPLATE( test_with_boost_units , Stepper , boost_unit_steppers )
{
    namespace fusion = boost::fusion;
    namespace si = boost::units::si;
    Stepper s;

    coor_type q = fusion::make_vector( 1.0 * si::meter );
    mom_type p = fusion::make_vector( 2.0 * si::meter_per_second );
    time_type t = 0.0 * si::second;
    time_type dt = 0.1 * si::second;

    coor_type q1 = q , q2 = q;
    mom_type p1 = p , p2 = p;

    s.do_step( oscillator_mom_func_units() , std::make_pair( boost::ref( q ) , boost::ref( p ) ) , t , dt );

    s.do_step( std::make_pair( oscillator_coor_func_units() , oscillator_mom_func_units() ) ,
               std::make_pair( boost::ref( q1 ) , boost::ref( p1 ) ) , t , dt );

    s.do_step( oscillator_mom_func_units() , q2 , p2 , t , dt );

    BOOST_CHECK_CLOSE( ( fusion::at_c< 0 >( q  ).value() ) , ( fusion::at_c< 0 >( q1 ).value() ) , 1.0e-14 );
    BOOST_CHECK_CLOSE( ( fusion::at_c< 0 >( q1 ).value() ) , ( fusion::at_c< 0 >( q2 ).value() ) , 1.0e-14 );
    BOOST_CHECK_CLOSE( ( fusion::at_c< 0 >( p  ).value() ) , ( fusion::at_c< 0 >( p1 ).value() ) , 1.0e-14 );
    BOOST_CHECK_CLOSE( ( fusion::at_c< 0 >( p1 ).value() ) , ( fusion::at_c< 0 >( p2 ).value() ) , 1.0e-14 );
}





BOOST_AUTO_TEST_SUITE_END()
