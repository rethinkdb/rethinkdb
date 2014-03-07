/*
  [auto_generated]
  libs/numeric/odeint/test/dummy_boost_units.hpp

  [begin_description]
  tba.
  [end_description]

  Copyright 2009-2012 Karsten Ahnert
  Copyright 2009-2012 Mario Mulansky

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or
  copy at http://www.boost.org/LICENSE_1_0.txt)
*/


#ifndef LIBS_NUMERIC_ODEINT_TEST_DUMMY_BOOST_UNITS_HPP_DEFINED
#define LIBS_NUMERIC_ODEINT_TEST_DUMMY_BOOST_UNITS_HPP_DEFINED

#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/time.hpp>
#include <boost/units/systems/si/velocity.hpp>
#include <boost/units/systems/si/acceleration.hpp>
#include <boost/units/systems/si/io.hpp>

#include <boost/fusion/container.hpp>



typedef double value_type;
typedef boost::units::quantity< boost::units::si::time , value_type > time_type;
typedef boost::units::quantity< boost::units::si::length , value_type > length_type;
typedef boost::units::quantity< boost::units::si::velocity , value_type > velocity_type;
typedef boost::units::quantity< boost::units::si::acceleration , value_type > acceleration_type;



struct oscillator_mom_func_units
{
    template< class Coor , class MomDeriv >
    void operator()( const Coor &q , MomDeriv &dp ) const
    {
        const boost::units::quantity< boost::units::si::frequency , value_type > omega = 1.0 * boost::units::si::hertz;
        boost::fusion::at_c< 0 >( dp ) = - omega * omega * boost::fusion::at_c< 0 >( q );
    }
};

struct oscillator_coor_func_units
{
    template< class Mom , class CoorDeriv >
    void operator()( const Mom &p , CoorDeriv &dq ) const
    {
        boost::fusion::at_c< 0 >( dq ) = boost::fusion::at_c< 0 >( p );
    }
};


#endif // LIBS_NUMERIC_ODEINT_TEST_DUMMY_BOOST_UNITS_HPP_DEFINED
