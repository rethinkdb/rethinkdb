/*
 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#include <iostream>
#include <vector>


/* WARNING: Compilation in debug mode might consume enormous memory 
   (e.g. ~2GB on gcc 4.4 ) 
*/

// first increase fusion macro variables (now done by odeint itself)
//#define BOOST_FUSION_INVOKE_MAX_ARITY 15
//#define BOOST_RESULT_OF_NUM_ARGS 15

//[ units_define_basic_quantities

#include <boost/numeric/odeint.hpp>
#include <boost/numeric/odeint/algebra/fusion_algebra.hpp>

#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/time.hpp>
#include <boost/units/systems/si/velocity.hpp>
#include <boost/units/systems/si/acceleration.hpp>
#include <boost/units/systems/si/io.hpp>

#include <boost/fusion/container.hpp>

using namespace std;
using namespace boost::numeric::odeint;
namespace fusion = boost::fusion;
namespace units = boost::units;
namespace si = boost::units::si;

typedef units::quantity< si::time , double > time_type;
typedef units::quantity< si::length , double > length_type;
typedef units::quantity< si::velocity , double > velocity_type;
typedef units::quantity< si::acceleration , double > acceleration_type;
typedef units::quantity< si::frequency , double > frequency_type;

typedef fusion::vector< length_type , velocity_type > state_type;
typedef fusion::vector< velocity_type , acceleration_type > deriv_type;
//]



//[ units_define_ode
struct oscillator
{
    frequency_type m_omega;

    oscillator( const frequency_type &omega = 1.0 * si::hertz ) : m_omega( omega ) { }

    void operator()( const state_type &x , deriv_type &dxdt , time_type t ) const
    {
        fusion::at_c< 0 >( dxdt ) = fusion::at_c< 1 >( x );
        fusion::at_c< 1 >( dxdt ) = - m_omega * m_omega * fusion::at_c< 0 >( x );
    }
};
//]


//[ units_observer
struct streaming_observer
{
    std::ostream& m_out;

    streaming_observer( std::ostream &out ) : m_out( out ) { }

    struct write_element
    {
        std::ostream &m_out;
        write_element( std::ostream &out ) : m_out( out ) { };

        template< class T >
        void operator()( const T &t ) const
        {
            m_out << "\t" << t;
        }
    };

    template< class State , class Time >
    void operator()( const State &x , const Time &t ) const
    {
        m_out << t;
        fusion::for_each( x , write_element( m_out ) );
        m_out << "\n";
    }
};
//]


int main( int argc , char**argv )
{
//    typedef dense_output_runge_kutta
//    <
//        controlled_runge_kutta
//        <
//            runge_kutta_dopri5< state_type , double , deriv_type , time_type , fusion_algebra >
//        >
//    > stepper_type;

    //[ units_define_stepper
    typedef runge_kutta_dopri5< state_type , double , deriv_type , time_type , fusion_algebra > stepper_type;

    state_type x( 1.0 * si::meter , 0.0 * si::meter_per_second );

    integrate_const( make_dense_output( 1.0e-6 , 1.0e-6 , stepper_type() ) , oscillator( 2.0 * si::hertz ) , x , 0.0 * si::second , 100.0 * si::second , 0.1 * si::second , streaming_observer( cout ) );
    //]

    return 0;
}
