// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2009 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** 
\file
    
\brief test_trig.cpp

\details
Test trigonometric functions.

Output:
@verbatim
@endverbatim
**/

#include <cmath>
#include <boost/units/cmath.hpp>
#include <boost/units/io.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/dimensionless.hpp>
#include <boost/units/systems/angle/degrees.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using boost::units::si::radians;
using boost::units::si::si_dimensionless;
using boost::units::degree::degrees;
BOOST_UNITS_STATIC_CONSTANT(degree_dimensionless, boost::units::degree::dimensionless);
using boost::units::si::meters;
BOOST_UNITS_STATIC_CONSTANT(heterogeneous_dimensionless, boost::units::reduce_unit<boost::units::si::dimensionless>::type);

BOOST_AUTO_TEST_CASE(test_sin) {
    BOOST_CHECK_EQUAL(boost::units::sin(2.0 * radians), std::sin(2.0) * si_dimensionless);
    BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(boost::units::sin(15.0 * degrees)), 0.2588, 0.0004);
}

BOOST_AUTO_TEST_CASE(test_cos) {
    BOOST_CHECK_EQUAL(boost::units::cos(2.0 * radians), std::cos(2.0) * si_dimensionless);
    BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(boost::units::cos(75.0 * degrees)), 0.2588, 0.0004);
}

BOOST_AUTO_TEST_CASE(test_tan) {
    BOOST_CHECK_EQUAL(boost::units::tan(2.0 * radians), std::tan(2.0) * si_dimensionless);
    BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(boost::units::tan(45.0 * degrees)), 1.0, 0.0001);
}

BOOST_AUTO_TEST_CASE(test_asin) {
    BOOST_CHECK_EQUAL(boost::units::asin(0.2 * si_dimensionless), std::asin(0.2) * radians);
    BOOST_CHECK_CLOSE_FRACTION(boost::units::asin(0.5 * degree_dimensionless).value(), 30.0, 0.0001);
    BOOST_CHECK_EQUAL(boost::units::asin(0.2 * heterogeneous_dimensionless).value(), std::asin(0.2));
}

BOOST_AUTO_TEST_CASE(test_acos) {
    BOOST_CHECK_EQUAL(boost::units::acos(0.2 * si_dimensionless), std::acos(0.2) * radians);
    BOOST_CHECK_CLOSE_FRACTION(boost::units::acos(0.5 * degree_dimensionless).value(), 60.0, 0.0001);
    BOOST_CHECK_EQUAL(boost::units::acos(0.2 * heterogeneous_dimensionless).value(), std::acos(0.2));
}

BOOST_AUTO_TEST_CASE(test_atan) {
    BOOST_CHECK_EQUAL(boost::units::atan(0.2 * si_dimensionless), std::atan(0.2) * radians);
    BOOST_CHECK_CLOSE_FRACTION(boost::units::atan(1.0 * degree_dimensionless).value(), 45.0, 0.0001);
    BOOST_CHECK_EQUAL(boost::units::atan(0.2 * heterogeneous_dimensionless).value(), std::atan(0.2));
}

BOOST_AUTO_TEST_CASE(test_atan2) {
    BOOST_CHECK_EQUAL(boost::units::atan2(0.2 * si_dimensionless, 0.3 * si_dimensionless), std::atan2(0.2, 0.3) * radians);
    BOOST_CHECK_EQUAL(boost::units::atan2(0.2 * meters, 0.3 * meters), std::atan2(0.2, 0.3) * radians);
    BOOST_CHECK_CLOSE_FRACTION(boost::units::atan2(0.8660*degree_dimensionless,0.5*degree_dimensionless).value(), 60., 0.0002);
    BOOST_CHECK_EQUAL(boost::units::atan2(0.2 * heterogeneous_dimensionless, 0.3 * heterogeneous_dimensionless).value(), std::atan2(0.2, 0.3));
}
