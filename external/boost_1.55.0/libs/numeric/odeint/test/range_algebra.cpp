/*
 [auto_generated]
 libs/numeric/odeint/test/check_operations.cpp

 [begin_description]
 This file tests interaction between the algebras and the operations.
 [end_description]

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#define BOOST_TEST_MODULE odeint_standard_algebra

#include <cmath>
#include <complex>
#include <utility>
#include <functional>
#include <boost/array.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/time.hpp>
#include <boost/units/systems/si/velocity.hpp>
#include <boost/units/systems/si/io.hpp>

#include <boost/numeric/odeint/algebra/default_operations.hpp>
#include <boost/numeric/odeint/algebra/range_algebra.hpp>
#include <boost/numeric/odeint/algebra/array_algebra.hpp>

#include <boost/mpl/list.hpp>

namespace units = boost::units;
namespace si = boost::units::si;

using boost::numeric::odeint::default_operations;
using boost::numeric::odeint::range_algebra;
using boost::numeric::odeint::array_algebra;


BOOST_AUTO_TEST_SUITE( standard_algebra_test )

typedef boost::mpl::list< range_algebra , array_algebra > algebra_types;
range_algebra algebra;

BOOST_AUTO_TEST_CASE_TEMPLATE( for_each2 , algebra_type , algebra_types )
{
    algebra_type algebra;
    boost::array< double , 2 > x1 = {{ 1.0 , 1.0 }} , x2 = {{ 2.0 , 2.0 }};
    algebra.for_each2( x1 , x2 , default_operations::scale_sum1<>( 1.0 ) );
    BOOST_CHECK_CLOSE( x1[0] , 2.0 , 1.0e-10 );
    BOOST_CHECK_CLOSE( x1[1] , 2.0 , 1.0e-10 );
}



BOOST_AUTO_TEST_CASE_TEMPLATE( for_each3 , algebra_type , algebra_types )
{
    algebra_type algebra;
    boost::array< double , 2 > x1 = {{ 1.0 , 1.0 }} , x2 = {{ 2.0 , 2.0 }} , x3 = {{ 3.0 , 3.0 }};
    algebra.for_each3( x1 , x2 , x3 , default_operations::scale_sum2<>( 1.0 , 2.0 ) );
    BOOST_CHECK_CLOSE( x1[0] , 8.0 , 1.0e-10 );
    BOOST_CHECK_CLOSE( x1[1] , 8.0 , 1.0e-10 );
}


BOOST_AUTO_TEST_CASE_TEMPLATE( for_each4 , algebra_type , algebra_types )
{
    algebra_type algebra;
    boost::array< double , 2 > x1 = {{ 1.0 , 1.0 }} , x2 = {{ 2.0 , 2.0 }} , x3 = {{ 3.0 , 3.0 }} , x4 = {{ 4.0 , 4.0 }};
    algebra.for_each4( x1 , x2 , x3 , x4 , default_operations::scale_sum3<>( 1.0 , 2.0 , 3.0 ) );
    BOOST_CHECK_CLOSE( x1[0] , 20.0 , 1.0e-10 );
    BOOST_CHECK_CLOSE( x1[1] , 20.0 , 1.0e-10 );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( for_each5 , algebra_type , algebra_types )
{
    algebra_type algebra;
    boost::array< double , 2 > x1 = {{ 1.0 , 1.0 }} , x2 = {{ 2.0 , 2.0 }} , x3 = {{ 3.0 , 3.0 }} , x4 = {{ 4.0 , 4.0 }} , x5 = {{ 5.0 , 5.0 }};
    algebra.for_each5( x1 , x2 , x3 , x4 , x5 , default_operations::scale_sum4<>( 1.0 , 2.0 , 3.0 , 4.0 ) );
    BOOST_CHECK_CLOSE( x1[0] , 40.0 , 1.0e-10 );
    BOOST_CHECK_CLOSE( x1[1] , 40.0 , 1.0e-10 );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( for_each6 , algebra_type , algebra_types )
{
    algebra_type algebra;
    boost::array< double , 2 > x1 = {{ 1.0 , 1.0 }} , x2 = {{ 2.0 , 2.0 }} , x3 = {{ 3.0 , 3.0 }} , x4 = {{ 4.0 , 4.0 }} , x5 = {{ 5.0 , 5.0 }} , x6 = {{ 6.0 , 6.0 }};
    algebra.for_each6( x1 , x2 , x3 , x4 , x5 , x6 ,default_operations::scale_sum5<>( 1.0 , 2.0 , 3.0 , 4.0 , 5.0 ) );
    BOOST_CHECK_CLOSE( x1[0] , 70.0 , 1.0e-10 );
    BOOST_CHECK_CLOSE( x1[1] , 70.0 , 1.0e-10 );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( for_each7 , algebra_type , algebra_types )
{
    algebra_type algebra;
    boost::array< double , 2 > x1 = {{ 1.0 , 1.0 }} , x2 = {{ 2.0 , 2.0 }} , x3 = {{ 3.0 , 3.0 }} , x4 = {{ 4.0 , 4.0 }} , x5 = {{ 5.0 , 5.0 }} , x6 = {{ 6.0 , 6.0 }} , x7 = {{ 7.0 , 7.0 }};
    algebra.for_each7( x1 , x2 , x3 , x4 , x5 , x6 , x7 , default_operations::scale_sum6<>( 1.0 , 2.0 , 3.0 , 4.0 , 5.0 , 6.0 ) );
    BOOST_CHECK_CLOSE( x1[0] , 112.0 , 1.0e-10 );
    BOOST_CHECK_CLOSE( x1[1] , 112.0 , 1.0e-10 );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( for_each8 , algebra_type , algebra_types )
{
    algebra_type algebra;
    boost::array< double , 2 > x1 = {{ 1.0 , 1.0 }} , x2 = {{ 2.0 , 2.0 }} , x3 = {{ 3.0 , 3.0 }} , x4 = {{ 4.0 , 4.0 }} , x5 = {{ 5.0 , 5.0 }} , x6 = {{ 6.0 , 6.0 }} , x7 = {{ 7.0 , 7.0 }} , x8 = {{ 8.0 , 8.0 }};
    algebra.for_each8( x1 , x2 , x3 , x4 , x5 , x6 , x7 , x8 , default_operations::scale_sum7<>( 1.0 , 2.0 , 3.0 , 4.0 , 5.0 , 6.0 , 7.0 ) );
    BOOST_CHECK_CLOSE( x1[0] , 168.0 , 1.0e-10 );
    BOOST_CHECK_CLOSE( x1[1] , 168.0 , 1.0e-10 );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( reduce , algebra_type , algebra_types )
{
    algebra_type algebra;
    boost::array< double , 2 > x = {{ 1.25 , 2.25 }};
    double sum = algebra.reduce( x , std::plus< double >() , 0.0 );
    BOOST_CHECK_CLOSE( sum , 3.5 , 1.0e-10 );
    BOOST_CHECK_CLOSE( x[0] , 1.25 , 1.0e-10 );
    BOOST_CHECK_CLOSE( x[1] , 2.25 , 1.0e-10 );
}

struct adder
{
    template< typename Value , typename T1 , typename T2 >
    Value operator()( Value val , T1 t1 , T2 t2 ) const
    {
        return val + t1 + t2;
    }

    template< typename Value , typename T1 , typename T2 , typename T3 >
    Value operator()( Value val , T1 t1 , T2 t2 , T3 t3 ) const
    {
        return val + t1 + t2 + t3 ;
    }

    template< typename Value , typename T1 , typename T2 , typename T3 , typename T4 >
    Value operator()( Value val , T1 t1 , T2 t2 , T3 t3 , T4 t4 ) const
    {
        return val + t1 + t2 + t3 + t4 ;
    }

};

BOOST_AUTO_TEST_CASE( reduce2 )
{
    range_algebra algebra;
    boost::array< double , 2 > x1 = {{ 1.25 , 2.25 }} , x2 = {{ 0.5 , -1.0 }};
    double sum = algebra.reduce2( x1 , x2 , adder() , 0.0 );
    BOOST_CHECK_CLOSE( sum , 3.0 , 1.0e-10 );

}

BOOST_AUTO_TEST_CASE( reduce3 )
{
    range_algebra algebra;
    boost::array< double , 2 > x1 = {{ 1.25 , 2.25 }} , x2 = {{ 0.5 , -1.0 }} , x3 = {{ 5.0 , 10.0 }};
    double sum = algebra.reduce3( x1 , x2 , x3 , adder() , 0.0 );
    BOOST_CHECK_CLOSE( sum , 18.0 , 1.0e-10 );
}

BOOST_AUTO_TEST_CASE( reduce4 )
{
    range_algebra algebra;
    boost::array< double , 2 > x1 = {{ 1.25 , 2.25 }} , x2 = {{ 0.5 , -1.0 }} , x3 = {{ 5.0 , 10.0 }} , x4 = {{ 2.0 , -1.0 }};
    double sum = algebra.reduce4( x1 , x2 , x3 , x4 , adder() , 0.0 );
    BOOST_CHECK_CLOSE( sum , 19.0 , 1.0e-10 );
}


BOOST_AUTO_TEST_CASE( for_each2_with_units )
{
    range_algebra algebra;
    typedef units::quantity< si::time , double > time_type;
    typedef units::quantity< si::length , double > length_type;
    typedef units::quantity< si::velocity , double > velocity_type;
    boost::array< length_type , 2 > x1 = {{ 1.0 * si::meter , 1.0 * si::meter }};
    boost::array< velocity_type , 2 > x2 = {{ 2.0 * si::meter / si::seconds , 2.0 * si::meter / si::seconds }};
    algebra.for_each2( x1 , x2 , default_operations::scale_sum1< time_type >( 0.1 * si::second ) );
    BOOST_CHECK_CLOSE( x1[0].value() , 0.2 , 1.0e-10 );
    BOOST_CHECK_CLOSE( x1[1].value() , 0.2 , 1.0e-10 );
}


BOOST_AUTO_TEST_SUITE_END()
