/*
  [auto_generated]
  libs/numeric/odeint/test/default_operations.cpp

  [begin_description]
  This file tests default_operations.
  [end_description]

  Copyright 2009-2012 Karsten Ahnert
  Copyright 2009-2012 Mario Mulansky

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or
  copy at http://www.boost.org/LICENSE_1_0.txt)
*/


#define BOOST_TEST_MODULE odeint_standard_operations

#include <cmath>
#include <complex>

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/time.hpp>
#include <boost/units/systems/si/velocity.hpp>
#include <boost/units/systems/si/acceleration.hpp>
#include <boost/units/systems/si/io.hpp>

#include <boost/mpl/list.hpp>

#include <boost/numeric/odeint/algebra/default_operations.hpp>

namespace units = boost::units;
namespace si = boost::units::si;
namespace mpl = boost::mpl;
using boost::numeric::odeint::default_operations;


template< class Value > struct internal_value_type { typedef Value type; };
template< class T > struct internal_value_type< std::complex< T > > { typedef T type; };

template< class T > struct default_eps;
template<> struct default_eps< double > { static double def_eps( void ) { return 1.0e-10; } };
template<> struct default_eps< float > { static float def_eps( void ) { return 1.0e-5; } };


typedef units::unit< units::derived_dimension< units::time_base_dimension , 2 >::type , si::system > time_2;
typedef units::unit< units::derived_dimension< units::time_base_dimension , 3 >::type , si::system > time_3;
typedef units::unit< units::derived_dimension< units::time_base_dimension , 4 >::type , si::system > time_4;
typedef units::unit< units::derived_dimension< units::time_base_dimension , 5 >::type , si::system > time_5;
typedef units::unit< units::derived_dimension< units::time_base_dimension , 6 >::type , si::system > time_6;
typedef units::unit< units::derived_dimension< units::time_base_dimension , 7 >::type , si::system > time_7;

const time_2 second2 = si::second * si::second;
const time_3 second3 = second2 * si::second;
const time_4 second4 = second3 * si::second;
const time_5 second5 = second4 * si::second;
const time_6 second6 = second5 * si::second;
const time_7 second7 = second6 * si::second;




template< class Value , class Compare = typename internal_value_type< Value >::type >
struct double_fixture
{
    typedef Value value_type;
    typedef Compare compare_type;

    double_fixture( const compare_type &eps_ = default_eps< compare_type >::def_eps() )
        : m_eps( eps_ ) , res( 0.0 ) , x1( 1.0 ) , x2( 2.0 ) , x3( 3.0 ) , x4( 4.0 ) , x5( 5.0 ) , x6( 6.0 ) , x7( 7.0 ) , x8( 8.0 )
    {}

    ~double_fixture( void )
    {
        using std::abs;
        BOOST_CHECK_SMALL( abs( x1 - value_type( 1.0 ) ) , m_eps );
        BOOST_CHECK_SMALL( abs( x2 - value_type( 2.0 ) ) , m_eps );
        BOOST_CHECK_SMALL( abs( x3 - value_type( 3.0 ) ) , m_eps );
        BOOST_CHECK_SMALL( abs( x4 - value_type( 4.0 ) ) , m_eps );
        BOOST_CHECK_SMALL( abs( x5 - value_type( 5.0 ) ) , m_eps );
        BOOST_CHECK_SMALL( abs( x6 - value_type( 6.0 ) ) , m_eps );
        BOOST_CHECK_SMALL( abs( x7 - value_type( 7.0 ) ) , m_eps );
    }

    const compare_type m_eps;
    value_type res;
    value_type x1 , x2 , x3 , x4 , x5 , x6 , x7 , x8;
};

template< class Value , class Compare = typename internal_value_type< Value >::type >
struct unit_fixture
{
    typedef Value value_type;
    typedef Compare compare_type;
    typedef units::quantity< si::length , value_type > length_type;

    typedef units::quantity< si::time , value_type > time_type;
    typedef units::quantity< time_2 , value_type > time_2_type;
    typedef units::quantity< time_3 , value_type > time_3_type;
    typedef units::quantity< time_4 , value_type > time_4_type;
    typedef units::quantity< time_5 , value_type > time_5_type;
    typedef units::quantity< time_6 , value_type > time_6_type;
    typedef units::quantity< time_7 , value_type > time_7_type;

    typedef units::quantity< si::velocity , value_type > velocity_type;
    typedef units::quantity< si::acceleration , value_type > acceleration_type;

    unit_fixture( const compare_type &eps_ = default_eps< compare_type >::def_eps() )
        : m_eps( eps_ )
        , res( 0.0 * si::meter )
        ,   x( 1.0 * si::meter )
        , d1x( 2.0 * si::meter / si::second )
        , d2x( 3.0 * si::meter / si::second / si::second )
    {}

    ~unit_fixture( void )
    {
        using std::abs;
        BOOST_CHECK_SMALL( abs(   x.value() - value_type( 1.0 ) ) , m_eps );
        BOOST_CHECK_SMALL( abs( d1x.value() - value_type( 2.0 ) ) , m_eps );
        BOOST_CHECK_SMALL( abs( d2x.value() - value_type( 3.0 ) ) , m_eps );
    }

    compare_type m_eps;
    length_type res;
    length_type x;
    velocity_type d1x;
    acceleration_type d2x;
};


typedef mpl::list< float , double , std::complex< double > > test_types;

BOOST_AUTO_TEST_SUITE( check_operations_test )

using std::abs;

BOOST_AUTO_TEST_CASE_TEMPLATE( scale_sum2_test , T , test_types )
{
    typedef double_fixture< T > fix_type;
    fix_type f;
    typedef typename default_operations::scale_sum2< T , T > Op;
    Op op( 1.25 , 9.81 );
    op( f.res , f.x1 , f.x2 );
    BOOST_CHECK_SMALL( abs( f.res  - T( 20.87 ) ) , f.m_eps );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( scale_sum3_test , T , test_types )
{
    typedef double_fixture< T > fix_type;
    fix_type f;
    typedef default_operations::scale_sum3< T , T , T > Op;
    Op op( 1.25 , 9.81 , 0.87 );
    op( f.res , f.x1 , f.x2 , f.x3 );
    BOOST_CHECK_SMALL( abs( f.res - T( 23.48 ) ) , f.m_eps );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( scale_sum4_test , T , test_types  )
{
    typedef double_fixture< T > fix_type;
    fix_type f;
    typedef default_operations::scale_sum4< T , T , T , T > Op;
    Op op( 1.25 , 9.81 , 0.87 , -0.15 );
    op( f.res , f.x1 , f.x2 , f.x3 , f.x4 );
    BOOST_CHECK_SMALL( abs( f.res  - T( 22.88 ) ) , f.m_eps );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( scale_sum5_test , T , test_types )
{
    typedef double_fixture< T > fix_type;
    fix_type f;
    typedef default_operations::scale_sum5< T , T , T , T , T > Op;
    Op op( 1.25 , 9.81 , 0.87 , -0.15 , -3.3 );
    op( f.res , f.x1 , f.x2 , f.x3 , f.x4 , f.x5 );
    BOOST_CHECK_SMALL( abs( f.res  - T( 6.38 ) ) , f.m_eps );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( scale_sum6_test , T , test_types )
{
    typedef double_fixture< T > fix_type;
    fix_type f;
    typedef default_operations::scale_sum6< T , T , T , T , T , T > Op;
    Op op( 1.25 , 9.81 , 0.87 , -0.15 , -3.3 , 4.2 );
    op( f.res , f.x1 , f.x2 , f.x3 , f.x4 , f.x5 , f.x6 );
    BOOST_CHECK_SMALL( abs( f.res - T( 31.58 ) ) , f.m_eps );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( scale_sum7_test , T , test_types )
{
    typedef double_fixture< T > fix_type;
    fix_type f;
    typedef default_operations::scale_sum7< T , T , T , T , T , T , T > Op;
    Op op( 1.25 , 9.81 , 0.87 , -0.15 , -3.3 , 4.2 , -0.22 );
    op( f.res , f.x1 , f.x2 , f.x3 , f.x4 , f.x5 , f.x6 , f.x7 );
    BOOST_CHECK_SMALL( abs( f.res - T( 30.04 ) ) , f.m_eps );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( rel_error_test , T , test_types )
{
    typedef double_fixture< T > fix_type;
    fix_type f;
    f.res = -1.1;
    typedef default_operations::rel_error< T > Op;
    Op op( 0.1 , 0.2 , 0.15 , 0.12 );
    op( f.res , -f.x1 , -f.x2 );
    BOOST_CHECK_SMALL( abs( f.res - T( 6.17978 ) ) , typename fix_type::compare_type( 1.0e-4 ) );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( maximum_test , T , test_types )
{
    typedef double_fixture< T > fix_type;
    fix_type f;
    typedef default_operations::maximum< typename fix_type::compare_type > Op;
    Op op;
    f.res = op( f.x1 , f.x2 );
    BOOST_CHECK_SMALL( abs( f.res - T( 2.0 ) ) , f.m_eps );
}





BOOST_AUTO_TEST_CASE_TEMPLATE( scale_sum2_units_test , T , test_types )
{
    typedef unit_fixture< T > fix_type;
    typedef typename fix_type::value_type value_type;
    typedef typename fix_type::time_type time_type;
    typedef typename fix_type::time_2_type time_2_type;
    typedef typename fix_type::time_3_type time_3_type;
    typedef typename fix_type::time_4_type time_4_type;
    typedef typename fix_type::time_5_type time_5_type;
    typedef typename fix_type::time_6_type time_6_type;
    typedef typename fix_type::time_7_type time_7_type;

    fix_type f;
    typedef default_operations::scale_sum2< value_type , time_type > Op;
    Op op( 1.0 , time_type( 1.0 * si::second ) );
    op( f.res , f.x , f.d1x );
    BOOST_CHECK_SMALL( abs( f.res.value() - T( 3.0 ) ) , f.m_eps );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( scale_sum3_units_test , T , test_types )
{
    typedef unit_fixture< T > fix_type;
    typedef typename fix_type::value_type value_type;
    typedef typename fix_type::time_type time_type;
    typedef typename fix_type::time_2_type time_2_type;
    typedef typename fix_type::time_3_type time_3_type;
    typedef typename fix_type::time_4_type time_4_type;
    typedef typename fix_type::time_5_type time_5_type;
    typedef typename fix_type::time_6_type time_6_type;
    typedef typename fix_type::time_7_type time_7_type;

    fix_type f;
    typedef default_operations::scale_sum3< value_type , time_type , time_2_type > Op;
    Op op( 1.0 , time_type( 1.0 * si::second ) , time_2_type( 1.0 * second2 ) );
    op( f.res , f.x , f.d1x , f.d2x );
    BOOST_CHECK_SMALL( abs( f.res.value() - T( 6.0 ) ) , f.m_eps );
}




BOOST_AUTO_TEST_SUITE_END()
