/*
 [auto_generated]
 libs/numeric/odeint/examples/heun.cpp

 [begin_description]
 Examplary implementation of the method of Heun.
 [end_description]

 Copyright 2009-2011 Karsten Ahnert
 Copyright 2009-2011 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <iostream>


#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>

#include <boost/array.hpp>

#include <boost/numeric/odeint.hpp>






namespace fusion = boost::fusion;

//[ heun_define_coefficients
template< class Value = double >
struct heun_a1 : boost::array< Value , 1 > {
    heun_a1( void )
    {
        (*this)[0] = static_cast< Value >( 1 ) / static_cast< Value >( 3 );
    }
};

template< class Value = double >
struct heun_a2 : boost::array< Value , 2 >
{
    heun_a2( void )
    {
        (*this)[0] = static_cast< Value >( 0 );
        (*this)[1] = static_cast< Value >( 2 ) / static_cast< Value >( 3 );
    }
};


template< class Value = double >
struct heun_b : boost::array< Value , 3 >
{
    heun_b( void )
    {
        (*this)[0] = static_cast<Value>( 1 ) / static_cast<Value>( 4 );
        (*this)[1] = static_cast<Value>( 0 );
        (*this)[2] = static_cast<Value>( 3 ) / static_cast<Value>( 4 );
    }
};

template< class Value = double >
struct heun_c : boost::array< Value , 3 >
{
    heun_c( void )
    {
        (*this)[0] = static_cast< Value >( 0 );
        (*this)[1] = static_cast< Value >( 1 ) / static_cast< Value >( 3 );
        (*this)[2] = static_cast< Value >( 2 ) / static_cast< Value >( 3 );
    }
};
//]


//[ heun_stepper_definition
template<
    class State ,
    class Value = double ,
    class Deriv = State ,
    class Time = Value ,
    class Algebra = boost::numeric::odeint::range_algebra ,
    class Operations = boost::numeric::odeint::default_operations ,
    class Resizer = boost::numeric::odeint::initially_resizer
>
class heun : public
boost::numeric::odeint::explicit_generic_rk< 3 , 3 , State , Value , Deriv , Time ,
                                             Algebra , Operations , Resizer >
{

public:

    typedef boost::numeric::odeint::explicit_generic_rk< 3 , 3 , State , Value , Deriv , Time ,
                                                         Algebra , Operations , Resizer > stepper_base_type;

    typedef typename stepper_base_type::state_type state_type;
    typedef typename stepper_base_type::wrapped_state_type wrapped_state_type;
    typedef typename stepper_base_type::value_type value_type;
    typedef typename stepper_base_type::deriv_type deriv_type;
    typedef typename stepper_base_type::wrapped_deriv_type wrapped_deriv_type;
    typedef typename stepper_base_type::time_type time_type;
    typedef typename stepper_base_type::algebra_type algebra_type;
    typedef typename stepper_base_type::operations_type operations_type;
    typedef typename stepper_base_type::resizer_type resizer_type;
    typedef typename stepper_base_type::stepper_type stepper_type;

    heun( const algebra_type &algebra = algebra_type() )
    : stepper_base_type(
            fusion::make_vector(
                heun_a1<Value>() ,
                heun_a2<Value>() ) ,
            heun_b<Value>() , heun_c<Value>() , algebra )
    { }
};
//]


const double sigma = 10.0;
const double R = 28.0;
const double b = 8.0 / 3.0;

struct lorenz
{
    template< class State , class Deriv >
    void operator()( const State &x_ , Deriv &dxdt_ , double t ) const
    {
        typename boost::range_iterator< const State >::type x = boost::begin( x_ );
        typename boost::range_iterator< Deriv >::type dxdt = boost::begin( dxdt_ );

        dxdt[0] = sigma * ( x[1] - x[0] );
        dxdt[1] = R * x[0] - x[1] - x[0] * x[2];
        dxdt[2] = -b * x[2] + x[0] * x[1];
    }
};

struct streaming_observer
{
    std::ostream &m_out;
    streaming_observer( std::ostream &out ) : m_out( out ) { }
    template< typename State , typename Value >
    void operator()( const State &x , Value t ) const
    {
        m_out << t;
        for( size_t i=0 ; i<x.size() ; ++i ) m_out << "\t" << x[i];
        m_out << "\n";
    }
};



int main( int argc , char **argv )
{
    using namespace std;
    using namespace boost::numeric::odeint;


    //[ heun_example
    typedef boost::array< double , 3 > state_type;
    heun< state_type > h;
    state_type x = {{ 10.0 , 10.0 , 10.0 }};

    integrate_const( h , lorenz() , x , 0.0 , 100.0 , 0.01 ,
                     streaming_observer( std::cout ) );

    //]

    return 0;
}
