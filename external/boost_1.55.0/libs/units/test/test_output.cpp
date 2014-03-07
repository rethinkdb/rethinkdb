// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2009 Steven Watanabe
// Copyright Paul A. Bristow 2010
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** 
\file test_output.cpp
    
\brief 
Test unit and quantity printing
\details
Tests for output from various units, name, symbol and raw formats, and automatic prefixing in engineering and binary units.
**/

#include <boost/units/quantity.hpp>
#include <boost/units/io.hpp>
#include <boost/units/unit.hpp>
#include <boost/units/scale.hpp>
#include <boost/units/scaled_base_unit.hpp>
#include <boost/units/make_scaled_unit.hpp>
#include <boost/units/base_unit.hpp>
#include <boost/units/make_system.hpp>
#include <boost/units/absolute.hpp>
#include <boost/units/physical_dimensions/length.hpp>
#include <boost/units/physical_dimensions/time.hpp>
#include <boost/units/physical_dimensions/velocity.hpp>
#include <boost/units/physical_dimensions/volume.hpp>
#include <boost/units/physical_dimensions/acceleration.hpp>
#include <boost/units/physical_dimensions/area.hpp>

#include <boost/regex.hpp>

#include <sstream>
#include <boost/config.hpp>
#include <limits>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

struct meter_base_unit : boost::units::base_unit<meter_base_unit, boost::units::length_dimension, 1> {
    static const char* name() { return("meter"); }
    static const char* symbol() { return("m"); }
};

struct second_base_unit : boost::units::base_unit<second_base_unit, boost::units::time_dimension, 2> {
    static const char* name() { return("second"); }
    static const char* symbol() { return("s"); }
};

struct byte_base_unit : boost::units::base_unit<byte_base_unit, boost::units::dimensionless_type, 3> {
    static const char* name() { return("byte"); }
    static const char* symbol() { return("b"); }
};

typedef boost::units::make_system<meter_base_unit, second_base_unit>::type my_system;

typedef boost::units::unit<boost::units::length_dimension, my_system> length;
typedef boost::units::unit<boost::units::velocity_dimension, my_system> velocity;

typedef boost::units::make_scaled_unit<length, boost::units::scale<10, boost::units::static_rational<3> > >::type scaled_length;
typedef boost::units::make_scaled_unit<velocity, boost::units::scale<10, boost::units::static_rational<3> > >::type scaled_velocity1;

typedef boost::units::scaled_base_unit<second_base_unit, boost::units::scale<10, boost::units::static_rational<-3> > > millisecond_base_unit;

typedef boost::units::make_system<meter_base_unit, millisecond_base_unit>::type scaled_system;

typedef boost::units::unit<boost::units::time_dimension, scaled_system> scaled_time;
typedef boost::units::unit<boost::units::velocity_dimension, scaled_system> scaled_velocity2;

typedef boost::units::unit<boost::units::area_dimension, my_system> area;
typedef boost::units::make_scaled_unit<area, boost::units::scale<10, boost::units::static_rational<3> > >::type scaled_area;

typedef boost::units::make_scaled_unit<scaled_length, boost::units::scale<2, boost::units::static_rational<10> > >::type double_scaled_length;

typedef boost::units::scaled_base_unit<meter_base_unit, boost::units::scale<100, boost::units::static_rational<1> > > scaled_length_base_unit;
namespace boost {
namespace units {
template<>
struct base_unit_info<scaled_length_base_unit> {
    static const char* symbol() { return("scm"); }
    static const char* name() { return("scaled_meter"); }
};
}
}
typedef boost::units::scaled_base_unit<scaled_length_base_unit, boost::units::scale<10, boost::units::static_rational<3> > > double_scaled_length_base_unit;
typedef double_scaled_length_base_unit::unit_type double_scaled_length2;

typedef boost::units::reduce_unit<boost::units::unit<boost::units::volume_dimension, my_system> >::type custom1;

std::string name_string(const custom1&) { return("custom1"); }
std::string symbol_string(const custom1&) { return("c1"); }

typedef boost::units::reduce_unit<boost::units::unit<boost::units::acceleration_dimension, my_system> >::type custom2;

const char* name_string(const custom2&) { return("custom2"); }
const char* symbol_string(const custom2&) { return("c2"); }

typedef boost::units::make_scaled_unit<custom1, boost::units::scale<10, boost::units::static_rational<3> > >::type scaled_custom1;
typedef boost::units::make_scaled_unit<custom2, boost::units::scale<10, boost::units::static_rational<3> > >::type scaled_custom2;

#ifndef BOOST_NO_CWCHAR

#define BOOST_UNITS_TEST_OUTPUT(v, expected)                \
{                                                           \
    std::ostringstream ss;                                  \
    ss FORMATTERS << v;                                     \
    BOOST_CHECK_EQUAL(ss.str(), expected);                  \
}                                                           \
{                                                           \
    std::wostringstream ss;                                 \
    ss FORMATTERS << v;                                     \
    BOOST_CHECK(ss.str() == BOOST_PP_CAT(L, expected));     \
}

#define BOOST_UNITS_TEST_OUTPUT_REGEX(v, expected)          \
{                                                           \
    std::ostringstream ss;                                  \
    ss FORMATTERS << v;                                     \
    boost::regex r(expected);                               \
    BOOST_CHECK_MESSAGE(boost::regex_match(ss.str(), r),    \
        ss.str() + " does not match " + expected);          \
}                                                           \
{                                                           \
    std::wostringstream ss;                                 \
    ss FORMATTERS << v;                                     \
    boost::wregex r(BOOST_PP_CAT(L, expected));             \
    BOOST_CHECK(boost::regex_match(ss.str(), r));           \
}

#else

#define BOOST_UNITS_TEST_OUTPUT(v, expected)                \
{                                                           \
    std::ostringstream ss;                                  \
    ss FORMATTERS << v;                                     \
    BOOST_CHECK_EQUAL(ss.str(), expected);                  \
}

#define BOOST_UNITS_TEST_OUTPUT_REGEX(v, expected)          \
{                                                           \
    std::ostringstream ss;                                  \
    ss FORMATTERS << v;                                     \
    boost::regex r(expected);                               \
    BOOST_CHECK_MESSAGE(boost::regex_match(ss.str(), r),    \
        ss.str() + " does not match " + expected);          \
}

#endif

BOOST_AUTO_TEST_CASE(test_output_unit_symbol)
{  // base units using default symbol_format (no format specified) and no auto prefixing.
#define FORMATTERS
    BOOST_UNITS_TEST_OUTPUT(meter_base_unit::unit_type(), "m");
    BOOST_UNITS_TEST_OUTPUT(velocity(), "m s^-1");
    BOOST_UNITS_TEST_OUTPUT(scaled_length(), "km");
    BOOST_UNITS_TEST_OUTPUT(scaled_velocity1(), "k(m s^-1)");
    BOOST_UNITS_TEST_OUTPUT(millisecond_base_unit::unit_type(), "ms");
    BOOST_UNITS_TEST_OUTPUT(scaled_time(), "ms");
    BOOST_UNITS_TEST_OUTPUT(scaled_velocity2(), "m ms^-1");
    BOOST_UNITS_TEST_OUTPUT(area(), "m^2");
    BOOST_UNITS_TEST_OUTPUT(scaled_area(), "k(m^2)");
    BOOST_UNITS_TEST_OUTPUT(double_scaled_length(), "Kikm");
    BOOST_UNITS_TEST_OUTPUT(double_scaled_length2(), "kscm");
    BOOST_UNITS_TEST_OUTPUT(custom1(), "c1");
    BOOST_UNITS_TEST_OUTPUT(custom2(), "c2");
    BOOST_UNITS_TEST_OUTPUT(scaled_custom1(), "kc1");
    BOOST_UNITS_TEST_OUTPUT(scaled_custom2(), "kc2");
    BOOST_UNITS_TEST_OUTPUT(boost::units::absolute<meter_base_unit::unit_type>(), "absolute m");
#undef FORMATTERS
}

BOOST_AUTO_TEST_CASE(test_output_unit_raw)
{  // raw format specified
#define FORMATTERS << boost::units::raw_format
    BOOST_UNITS_TEST_OUTPUT(meter_base_unit::unit_type(), "m");
    BOOST_UNITS_TEST_OUTPUT(velocity(), "m s^-1");
    BOOST_UNITS_TEST_OUTPUT(scaled_length(), "km");
    BOOST_UNITS_TEST_OUTPUT(scaled_velocity1(), "k(m s^-1)");
    BOOST_UNITS_TEST_OUTPUT(millisecond_base_unit::unit_type(), "ms");
    BOOST_UNITS_TEST_OUTPUT(scaled_time(), "ms");
    BOOST_UNITS_TEST_OUTPUT(scaled_velocity2(), "m ms^-1");
    BOOST_UNITS_TEST_OUTPUT(area(), "m^2");
    BOOST_UNITS_TEST_OUTPUT(scaled_area(), "k(m^2)");
    BOOST_UNITS_TEST_OUTPUT(double_scaled_length(), "Kikm");
    BOOST_UNITS_TEST_OUTPUT(double_scaled_length2(), "kscm");
    // when using raw format, we ignore the user defined overloads
    BOOST_UNITS_TEST_OUTPUT(custom1(), "m^3");
    BOOST_UNITS_TEST_OUTPUT(custom2(), "m s^-2");
    BOOST_UNITS_TEST_OUTPUT(scaled_custom1(), "k(m^3)");
    BOOST_UNITS_TEST_OUTPUT(scaled_custom2(), "k(m s^-2)");
    BOOST_UNITS_TEST_OUTPUT(boost::units::absolute<meter_base_unit::unit_type>(), "absolute m");
#undef FORMATTERS
}

BOOST_AUTO_TEST_CASE(test_output_unit_name)
{  // name format specified.
#define FORMATTERS << boost::units::name_format
    BOOST_UNITS_TEST_OUTPUT(meter_base_unit::unit_type(), "meter");
    BOOST_UNITS_TEST_OUTPUT(velocity(), "meter second^-1");
    BOOST_UNITS_TEST_OUTPUT(scaled_length(), "kilometer");
    BOOST_UNITS_TEST_OUTPUT(scaled_velocity1(), "kilo(meter second^-1)");
    BOOST_UNITS_TEST_OUTPUT(millisecond_base_unit::unit_type(), "millisecond");
    BOOST_UNITS_TEST_OUTPUT(scaled_time(), "millisecond");
    BOOST_UNITS_TEST_OUTPUT(scaled_velocity2(), "meter millisecond^-1");
    BOOST_UNITS_TEST_OUTPUT(area(), "meter^2");
    BOOST_UNITS_TEST_OUTPUT(scaled_area(), "kilo(meter^2)");
    BOOST_UNITS_TEST_OUTPUT(double_scaled_length(), "kibikilometer");
    BOOST_UNITS_TEST_OUTPUT(double_scaled_length2(), "kiloscaled_meter");
    BOOST_UNITS_TEST_OUTPUT(custom1(), "custom1");
    BOOST_UNITS_TEST_OUTPUT(custom2(), "custom2");
    BOOST_UNITS_TEST_OUTPUT(scaled_custom1(), "kilocustom1");
    BOOST_UNITS_TEST_OUTPUT(scaled_custom2(), "kilocustom2");
    BOOST_UNITS_TEST_OUTPUT(boost::units::absolute<meter_base_unit::unit_type>(), "absolute meter");
#undef FORMATTERS
}


BOOST_AUTO_TEST_CASE(test_output_quantity_symbol)
{ // quantity symbols using default format.
#define FORMATTERS
    BOOST_UNITS_TEST_OUTPUT(1.5*meter_base_unit::unit_type(), "1.5 m");
    BOOST_UNITS_TEST_OUTPUT(1.5*velocity(), "1.5 m s^-1");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_length(), "1.5 km");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_velocity1(), "1.5 k(m s^-1)");
    BOOST_UNITS_TEST_OUTPUT(1.5*millisecond_base_unit::unit_type(), "1.5 ms");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_time(), "1.5 ms");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_velocity2(), "1.5 m ms^-1");
    BOOST_UNITS_TEST_OUTPUT(1.5*area(), "1.5 m^2");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_area(), "1.5 k(m^2)");
    BOOST_UNITS_TEST_OUTPUT(1.5*double_scaled_length(), "1.5 Kikm");
    BOOST_UNITS_TEST_OUTPUT(1.5*double_scaled_length2(), "1.5 kscm");
    BOOST_UNITS_TEST_OUTPUT(1.5*custom1(), "1.5 c1");
    BOOST_UNITS_TEST_OUTPUT(1.5*custom2(), "1.5 c2");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_custom1(), "1.5 kc1");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_custom2(), "1.5 kc2");
    BOOST_UNITS_TEST_OUTPUT(1.5*boost::units::absolute<meter_base_unit::unit_type>(), "1.5 absolute m");
    BOOST_UNITS_TEST_OUTPUT(pow(2., 10) * byte_base_unit::unit_type(), "1024 b");

#undef FORMATTERS
}

BOOST_AUTO_TEST_CASE(test_output_quantity_raw)
{ // quantity symbols using raw format.
#define FORMATTERS << boost::units::raw_format
    BOOST_UNITS_TEST_OUTPUT(1.5*meter_base_unit::unit_type(), "1.5 m");
    BOOST_UNITS_TEST_OUTPUT(1.5*velocity(), "1.5 m s^-1");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_length(), "1.5 km");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_velocity1(), "1.5 k(m s^-1)");
    BOOST_UNITS_TEST_OUTPUT(1.5*millisecond_base_unit::unit_type(), "1.5 ms");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_time(), "1.5 ms");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_velocity2(), "1.5 m ms^-1");
    BOOST_UNITS_TEST_OUTPUT(1.5*area(), "1.5 m^2");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_area(), "1.5 k(m^2)");
    BOOST_UNITS_TEST_OUTPUT(1.5*double_scaled_length(), "1.5 Kikm");
    BOOST_UNITS_TEST_OUTPUT(1.5*double_scaled_length2(), "1.5 kscm");
    // when using raw format, we ignore the user defined overloads
    BOOST_UNITS_TEST_OUTPUT(1.5*custom1(), "1.5 m^3");
    BOOST_UNITS_TEST_OUTPUT(1.5*custom2(), "1.5 m s^-2");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_custom1(), "1.5 k(m^3)");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_custom2(), "1.5 k(m s^-2)");
    BOOST_UNITS_TEST_OUTPUT(1.5*boost::units::absolute<meter_base_unit::unit_type>(), "1.5 absolute m");
#undef FORMATTERS
}

BOOST_AUTO_TEST_CASE(test_output_quantity_name)
{ // // quantity symbols using name format.
#define FORMATTERS << boost::units::name_format
    BOOST_UNITS_TEST_OUTPUT(1.5*meter_base_unit::unit_type(), "1.5 meter");
    BOOST_UNITS_TEST_OUTPUT(1.5*velocity(), "1.5 meter second^-1");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_length(), "1.5 kilometer");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_velocity1(), "1.5 kilo(meter second^-1)");
    BOOST_UNITS_TEST_OUTPUT(1.5*millisecond_base_unit::unit_type(), "1.5 millisecond");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_time(), "1.5 millisecond");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_velocity2(), "1.5 meter millisecond^-1");
    BOOST_UNITS_TEST_OUTPUT(1.5*area(), "1.5 meter^2");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_area(), "1.5 kilo(meter^2)");
    BOOST_UNITS_TEST_OUTPUT(1.5*double_scaled_length(), "1.5 kibikilometer");
    BOOST_UNITS_TEST_OUTPUT(1.5*double_scaled_length2(), "1.5 kiloscaled_meter");
    BOOST_UNITS_TEST_OUTPUT(1.5*custom1(), "1.5 custom1");
    BOOST_UNITS_TEST_OUTPUT(1.5*custom2(), "1.5 custom2");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_custom1(), "1.5 kilocustom1");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_custom2(), "1.5 kilocustom2");
    BOOST_UNITS_TEST_OUTPUT(1.5*boost::units::absolute<meter_base_unit::unit_type>(), "1.5 absolute meter");
#undef FORMATTERS
}

BOOST_AUTO_TEST_CASE(test_output_autoprefixed_quantity_name)
{ // Engineering autoprefix, with name format.
#define FORMATTERS << boost::units::name_format << boost::units::engineering_prefix 
  // Single base unit like meter.
    BOOST_UNITS_TEST_OUTPUT(1.5*meter_base_unit::unit_type(), "1.5 meter");
    BOOST_UNITS_TEST_OUTPUT(1500.0*meter_base_unit::unit_type(), "1.5 kilometer");
    BOOST_UNITS_TEST_OUTPUT(1.5e7*meter_base_unit::unit_type(), "15 megameter");
    BOOST_UNITS_TEST_OUTPUT(1.5e-3*meter_base_unit::unit_type(), "1.5 millimeter");
    BOOST_UNITS_TEST_OUTPUT(1.5e-9*meter_base_unit::unit_type(), "1.5 nanometer");
    BOOST_UNITS_TEST_OUTPUT(1.5e-8*meter_base_unit::unit_type(), "15 nanometer");
    BOOST_UNITS_TEST_OUTPUT(1.5e-10*meter_base_unit::unit_type(), "150 picometer");
    BOOST_UNITS_TEST_OUTPUT(0.0000000012345 * meter_base_unit::unit_type(), "1.2345 nanometer");

  // Too small or large for a multiple name.
    BOOST_UNITS_TEST_OUTPUT_REGEX(9.99999e-25 * meter_base_unit::unit_type(), "9\\.99999e-0?25 meter"); // Just too small for multiple.
    BOOST_UNITS_TEST_OUTPUT_REGEX(1e+28 * meter_base_unit::unit_type(), "1e\\+0?28 meter"); // Just too large for multiple.
    BOOST_UNITS_TEST_OUTPUT_REGEX(1.5e-25 * meter_base_unit::unit_type(), "1\\.5e-0?25 meter"); // Too small for multiple.
    BOOST_UNITS_TEST_OUTPUT_REGEX(1.5e+28 * meter_base_unit::unit_type(), "1\\.5e\\+0?28 meter"); // Too large for multiple.
  // Too 'biggest or too smallest'.
    BOOST_UNITS_TEST_OUTPUT_REGEX(std::numeric_limits<float>::max()*meter_base_unit::unit_type(), "3\\.40282e\\+0?38 meter");
    BOOST_UNITS_TEST_OUTPUT_REGEX(std::numeric_limits<float>::min()*meter_base_unit::unit_type(), "1\\.17549e-0?38 meter");
    BOOST_UNITS_TEST_OUTPUT(std::numeric_limits<double>::max()*meter_base_unit::unit_type(), "1.79769e+308 meter");
    BOOST_UNITS_TEST_OUTPUT(std::numeric_limits<double>::min()*meter_base_unit::unit_type(), "2.22507e-308 meter");
   // Infinity and NaN
    BOOST_UNITS_TEST_OUTPUT_REGEX(std::numeric_limits<float>::infinity()*meter_base_unit::unit_type(), "(1\\.#INF|inf|INF) meter");
    BOOST_UNITS_TEST_OUTPUT_REGEX(-std::numeric_limits<float>::infinity()*meter_base_unit::unit_type(), "-(1\\.#INF|inf|INF) meter");
    BOOST_UNITS_TEST_OUTPUT_REGEX(std::numeric_limits<double>::quiet_NaN()*meter_base_unit::unit_type(), "(1\\.#QNAN|nan|NaNQ) meter");
    BOOST_UNITS_TEST_OUTPUT_REGEX(-std::numeric_limits<double>::quiet_NaN()*meter_base_unit::unit_type(), "-?(1\\.#IND|nan|NaNQ) meter");

    BOOST_UNITS_TEST_OUTPUT(1.5*velocity(), "1.5 meter second^-1");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_length(), "1.5 kilometer");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_velocity1(), "1.5 kilo(meter second^-1)");
    BOOST_UNITS_TEST_OUTPUT(1.5*millisecond_base_unit::unit_type(), "1.5 millisecond");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_time(), "1.5 millisecond");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_velocity2(), "1.5 meter millisecond^-1");
    BOOST_UNITS_TEST_OUTPUT(1.5*area(), "1.5 meter^2");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_area(), "1.5 kilo(meter^2)");
    BOOST_UNITS_TEST_OUTPUT(1.5*double_scaled_length(), "1.536 megameter"); // 1.5 * 2^10 = 1.5 * 1024 = 1.536
    BOOST_UNITS_TEST_OUTPUT(1.5*double_scaled_length2(), "1.5 kiloscaled_meter");
    BOOST_UNITS_TEST_OUTPUT(1.5*custom1(), "1.5 custom1");
    BOOST_UNITS_TEST_OUTPUT(1.5*custom2(), "1.5 custom2");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_custom1(), "1.5 kilocustom1");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_custom2(), "1.5 kilocustom2");
    BOOST_UNITS_TEST_OUTPUT(1.5*boost::units::absolute<meter_base_unit::unit_type>(), "1.5 absolute meter");
    BOOST_UNITS_TEST_OUTPUT(pow(2., 10) * byte_base_unit::unit_type(), "1.024 kilobyte");

    BOOST_UNITS_TEST_OUTPUT(1.5, "1.5"); // scalar.
    BOOST_UNITS_TEST_OUTPUT(1567., "1567"); // scalars are *not* autoprefixed.
    BOOST_UNITS_TEST_OUTPUT(0.00015, "0.00015"); // scalars are *not* autoprefixed.
    BOOST_UNITS_TEST_OUTPUT(-1.5, "-1.5"); // scalar.
    BOOST_UNITS_TEST_OUTPUT(-1567., "-1567"); // scalars are *not* autoprefixed.
    BOOST_UNITS_TEST_OUTPUT(-0.00015, "-0.00015"); // scalars are *not* autoprefixed.
#undef FORMATTERS
}

BOOST_AUTO_TEST_CASE(test_output_autoprefixed_quantity_symbol)
{ // Engineering autoprefix, with symbol format.
#define FORMATTERS << boost::units::symbol_format << boost::units::engineering_prefix 
  // Single base unit like m.
    BOOST_UNITS_TEST_OUTPUT(1.5*meter_base_unit::unit_type(), "1.5 m");
    BOOST_UNITS_TEST_OUTPUT(1500.0*meter_base_unit::unit_type(), "1.5 km");
    BOOST_UNITS_TEST_OUTPUT(1.5e7*meter_base_unit::unit_type(), "15 Mm");
    BOOST_UNITS_TEST_OUTPUT(1.5e-3*meter_base_unit::unit_type(), "1.5 mm");
    BOOST_UNITS_TEST_OUTPUT(1.5e-9*meter_base_unit::unit_type(), "1.5 nm");
    BOOST_UNITS_TEST_OUTPUT(1.5e-8*meter_base_unit::unit_type(), "15 nm");
    BOOST_UNITS_TEST_OUTPUT(1.5e-10*meter_base_unit::unit_type(), "150 pm");
  // Too small or large for a multiple name.
    BOOST_UNITS_TEST_OUTPUT_REGEX(9.99999e-25 * meter_base_unit::unit_type(), "9\\.99999e-0?25 m"); // Just too small for multiple.
    BOOST_UNITS_TEST_OUTPUT_REGEX(1e+28 * meter_base_unit::unit_type(), "1e\\+0?28 m"); // Just too large for multiple.
    BOOST_UNITS_TEST_OUTPUT_REGEX(1.5e-25 * meter_base_unit::unit_type(), "1\\.5e-0?25 m"); // Too small for multiple.
    BOOST_UNITS_TEST_OUTPUT_REGEX(1.5e+28 * meter_base_unit::unit_type(), "1\\.5e\\+0?28 m"); // Too large for multiple.
  // 
    BOOST_UNITS_TEST_OUTPUT_REGEX(std::numeric_limits<float>::max()*meter_base_unit::unit_type(), "3\\.40282e\\+0?38 m");
    BOOST_UNITS_TEST_OUTPUT_REGEX(std::numeric_limits<float>::min()*meter_base_unit::unit_type(), "1\\.17549e-0?38 m");
    BOOST_UNITS_TEST_OUTPUT(std::numeric_limits<double>::max()*meter_base_unit::unit_type(), "1.79769e+308 m");
    BOOST_UNITS_TEST_OUTPUT(std::numeric_limits<double>::min()*meter_base_unit::unit_type(), "2.22507e-308 m");

    BOOST_UNITS_TEST_OUTPUT(1.5*velocity(), "1.5 m s^-1");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_length(), "1.5 km");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_velocity1(), "1.5 k(m s^-1)");
    BOOST_UNITS_TEST_OUTPUT(1.5*millisecond_base_unit::unit_type(), "1.5 ms");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_time(), "1.5 ms");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_velocity2(), "1.5 m ms^-1");
    BOOST_UNITS_TEST_OUTPUT(1.5*area(), "1.5 m^2");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_area(), "1.5 k(m^2)");
    BOOST_UNITS_TEST_OUTPUT(1.5*double_scaled_length(), "1.536 Mm"); // 1.5 * 2^10 = 1.5 * 1024 = 1.536
    BOOST_UNITS_TEST_OUTPUT(1.5*double_scaled_length2(), "1.5 kscm");
    BOOST_UNITS_TEST_OUTPUT(1.5*custom1(), "1.5 c1");
    BOOST_UNITS_TEST_OUTPUT(1.5*custom2(), "1.5 c2");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_custom1(), "1.5 kc1");
    BOOST_UNITS_TEST_OUTPUT(1.5*scaled_custom2(), "1.5 kc2");
    BOOST_UNITS_TEST_OUTPUT(1.5*boost::units::absolute<meter_base_unit::unit_type>(), "1.5 absolute m");
    BOOST_UNITS_TEST_OUTPUT(pow(2., 10) * byte_base_unit::unit_type(), "1.024 kb");

#undef FORMATTERS
}

BOOST_AUTO_TEST_CASE(test_output_auto_binary_prefixed_quantity_symbol)
{ // Binary prefix with symbol format.
#define FORMATTERS << boost::units::symbol_format << boost::units::binary_prefix
    BOOST_UNITS_TEST_OUTPUT(1024 * byte_base_unit::unit_type(), "1 Kib");
    BOOST_UNITS_TEST_OUTPUT(pow(2., 20) * byte_base_unit::unit_type(), "1 Mib");
    BOOST_UNITS_TEST_OUTPUT(pow(2., 30) * byte_base_unit::unit_type(), "1 Gib");
    BOOST_UNITS_TEST_OUTPUT(pow(2., 40) * byte_base_unit::unit_type(), "1 Tib");
    BOOST_UNITS_TEST_OUTPUT(pow(2., 50) * byte_base_unit::unit_type(), "1 Pib");
    BOOST_UNITS_TEST_OUTPUT(pow(2., 60) * byte_base_unit::unit_type(), "1 Eib");
    BOOST_UNITS_TEST_OUTPUT(42, "42"); // integer scalar.
    BOOST_UNITS_TEST_OUTPUT(-42, "-42"); // integer scalar.
    BOOST_UNITS_TEST_OUTPUT(1567, "1567"); // scalars are *not* autoprefixed.
    BOOST_UNITS_TEST_OUTPUT(-1567, "-1567"); // scalars are *not* autoprefixed.
#undef FORMATTERS
}

BOOST_AUTO_TEST_CASE(test_output_auto_binary_prefixed_quantity_name)
{ // Binary prefix with name format.
  // http://physics.nist.gov/cuu/Units/binary.html
  // 1998 the International Electrotechnical Commission (IEC) approved 
  // IEC 60027-2, Second edition, 2000-11, Letter symbols to be used in electrical technology
  // - Part 2: Telecommunications and electronics.
#define FORMATTERS << boost::units::name_format << boost::units::binary_prefix
    BOOST_UNITS_TEST_OUTPUT(2048  * byte_base_unit::unit_type(), "2 kibibyte");
    BOOST_UNITS_TEST_OUTPUT(pow(2., 32) *byte_base_unit::unit_type(), "4 gibibyte");
    BOOST_UNITS_TEST_OUTPUT(pow(2., 41) *byte_base_unit::unit_type(), "2 tebibyte"); // http://en.wikipedia.org/wiki/Tebibyte
    BOOST_UNITS_TEST_OUTPUT(pow(2., 50) *byte_base_unit::unit_type(), "1 pebibyte"); 
    BOOST_UNITS_TEST_OUTPUT(pow(2., 60) *byte_base_unit::unit_type(), "1 exbibyte");
    BOOST_UNITS_TEST_OUTPUT(2048, "2048"); // scalars are *not* autoprefixed.
    BOOST_UNITS_TEST_OUTPUT(-4096, "-4096"); // scalars are *not* autoprefixed.
#undef FORMATTERS
}

// Tests on using more than one format or prefix - only the last specified should be used.
// (This may indicate a programming mistake, but it is ignored).
BOOST_AUTO_TEST_CASE(test_output_quantity_name_duplicate)
{ // Ensure that if more than one format specified, only the last is used.
#define FORMATTERS << boost::units::symbol_format << boost::units::name_format
    BOOST_UNITS_TEST_OUTPUT(1.5*meter_base_unit::unit_type(), "1.5 meter");
#undef FORMATTERS
}

BOOST_AUTO_TEST_CASE(test_output_quantity_symbol_duplicate)
{ // Ensure that if more than one format specified, only the last is used.
#define FORMATTERS << boost::units::name_format << boost::units::symbol_format 
    BOOST_UNITS_TEST_OUTPUT(1.5*meter_base_unit::unit_type(), "1.5 m");
#undef FORMATTERS
}

BOOST_AUTO_TEST_CASE(test_output_auto_binary_prefixed_quantity_name_duplicate)
{ // Ensure that if more than one auto prefix specified, only the last is used.
#define FORMATTERS << boost::units::name_format << boost::units::binary_prefix << boost::units::engineering_prefix
    BOOST_UNITS_TEST_OUTPUT(2048 * byte_base_unit::unit_type(), "2.048 kilobyte");
#undef FORMATTERS
}

BOOST_AUTO_TEST_CASE(test_output_auto_binary_prefixed_quantity_symbol_duplicate)
{ // Ensure that if more than one auto prefix specified, only the last is used.
#define FORMATTERS << boost::units::symbol_format << boost::units::engineering_prefix << boost::units::binary_prefix
    BOOST_UNITS_TEST_OUTPUT(2048 * byte_base_unit::unit_type(), "2 Kib");
#undef FORMATTERS
}
