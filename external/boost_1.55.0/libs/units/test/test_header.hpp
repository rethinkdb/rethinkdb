// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** 
\file
    
\brief test_header.hpp

\details
Unit system for test purposes.

Output:
@verbatim
@endverbatim
**/

#ifndef BOOST_UNITS_TEST_HEADER_HPP
#define BOOST_UNITS_TEST_HEADER_HPP

#include <boost/test/minimal.hpp>

#include <boost/units/base_dimension.hpp>
#include <boost/units/derived_dimension.hpp>
#include <boost/units/static_constant.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/io.hpp>
#include <boost/units/base_unit.hpp>
#include <boost/units/make_system.hpp>

#include <boost/units/physical_dimensions/length.hpp>
#include <boost/units/physical_dimensions/mass.hpp>
#include <boost/units/physical_dimensions/time.hpp>

#define BOOST_UNITS_CHECK_CLOSE(a, b) (BOOST_CHECK((std::abs((a) - (b)) < .0000001)))

namespace boost {

namespace units {

//struct length_base_dimension : boost::units::base_dimension<length_base_dimension,1> { };     ///> base dimension of length
//struct mass_base_dimension : boost::units::base_dimension<mass_base_dimension,2> { };         ///> base dimension of mass
//struct time_base_dimension : boost::units::base_dimension<time_base_dimension,3> { };         ///> base dimension of time

typedef length_base_dimension::dimension_type    length_dimension;
typedef mass_base_dimension::dimension_type      mass_dimension;
typedef time_base_dimension::dimension_type      time_dimension;

typedef derived_dimension<length_base_dimension,2>::type area_dimension;
typedef derived_dimension<mass_base_dimension,1,
                            length_base_dimension,2,
                            time_base_dimension,-2>::type  energy_dimension;
typedef derived_dimension<mass_base_dimension,-1,
                            length_base_dimension,-2,
                            time_base_dimension,2>::type   inverse_energy_dim;
typedef derived_dimension<length_base_dimension,1,
                            time_base_dimension,-1>::type  velocity_dimension;
typedef derived_dimension<length_base_dimension,3>::type volume_dimension;

/// placeholder class defining test unit system
struct length_unit : base_unit<length_unit, length_dimension, 4> {};
struct mass_unit : base_unit<mass_unit, mass_dimension, 5> {};
struct time_unit : base_unit<time_unit, time_dimension, 6> {};

typedef make_system<length_unit, mass_unit, time_unit>::type system;

/// unit typedefs
typedef unit<dimensionless_type,system>     dimensionless;

typedef unit<length_dimension,system>            length;
typedef unit<mass_dimension,system>              mass;
typedef unit<time_dimension,system>              time;

typedef unit<area_dimension,system>              area;
typedef unit<energy_dimension,system>            energy;
typedef unit<inverse_energy_dim,system>    inverse_energy;
typedef unit<velocity_dimension,system>          velocity;
typedef unit<volume_dimension,system>            volume;

/// unit constants 
BOOST_UNITS_STATIC_CONSTANT(meter,length);
BOOST_UNITS_STATIC_CONSTANT(meters,length);
BOOST_UNITS_STATIC_CONSTANT(kilogram,mass);
BOOST_UNITS_STATIC_CONSTANT(kilograms,mass);
BOOST_UNITS_STATIC_CONSTANT(second,time);
BOOST_UNITS_STATIC_CONSTANT(seconds,time);

BOOST_UNITS_STATIC_CONSTANT(square_meter,area);
BOOST_UNITS_STATIC_CONSTANT(square_meters,area);
BOOST_UNITS_STATIC_CONSTANT(joule,energy);
BOOST_UNITS_STATIC_CONSTANT(joules,energy);
BOOST_UNITS_STATIC_CONSTANT(meter_per_second,velocity);
BOOST_UNITS_STATIC_CONSTANT(meters_per_second,velocity);
BOOST_UNITS_STATIC_CONSTANT(cubic_meter,volume);
BOOST_UNITS_STATIC_CONSTANT(cubic_meters,volume);

template<> struct base_unit_info<length_unit>
{
    static std::string name()               { return "meter"; }
    static std::string symbol()             { return "m"; }
};
//]

template<> struct base_unit_info<mass_unit>
{
    static std::string name()               { return "kilogram"; }
    static std::string symbol()             { return "kg"; }
};

template<> struct base_unit_info<time_unit>
{
    static std::string name()               { return "second"; }
    static std::string symbol()             { return "s"; }
};

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_TEST_HEADER_HPP
