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
    
\brief test_conversion.cpp

\details
Test conversion between quantities.

Output:
@verbatim
@endverbatim
**/

#include <boost/units/quantity.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/systems/cgs.hpp>

#include <iostream>

#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>

#define BOOST_UNITS_CHECK_CLOSE(a, b) BOOST_CHECK_CLOSE_FRACTION(a, b, .0000001)

namespace bu = boost::units;

typedef bu::si::length  si_length;
typedef bu::si::time    si_time;
typedef bu::si::mass    si_mass;
typedef bu::si::area    si_area;

typedef bu::cgs::length cgs_length;
typedef bu::cgs::time   cgs_time;
typedef bu::cgs::mass   cgs_mass;
typedef bu::cgs::area   cgs_area;

typedef bu::multiply_typeof_helper<si_length, cgs_length>::type     mixed_length;
typedef bu::multiply_typeof_helper<si_time, cgs_time>::type         mixed_time;

typedef bu::divide_typeof_helper<bu::multiply_typeof_helper<si_mass,cgs_area>::type, mixed_time>::type  mixed_energy_1;
typedef bu::divide_typeof_helper<bu::multiply_typeof_helper<cgs_mass,mixed_length>::type, 
                                 bu::multiply_typeof_helper<cgs_time,cgs_time>::type >::type            mixed_energy_2;

BOOST_AUTO_TEST_CASE(test_conversion) {
    BOOST_CHECK_EQUAL(1, 1);
    bu::quantity<mixed_length> a1(2.0 * mixed_length());
    bu::quantity<si_area> a2(a1);

    BOOST_UNITS_CHECK_CLOSE(a2.value(), .02);

    bu::quantity<mixed_length> a3(a2);

    BOOST_UNITS_CHECK_CLOSE(a3.value(), 2.0);

    bu::quantity<mixed_energy_1> e1(2.0 * mixed_energy_1());
    bu::quantity<mixed_energy_2> e2(e1);

    BOOST_UNITS_CHECK_CLOSE(e2.value(), 20.0);

    bu::quantity<bu::si::energy> e3(e1);
    BOOST_UNITS_CHECK_CLOSE(e3.value(), .0002);
    bu::quantity<mixed_energy_2> e4(e3);
    BOOST_UNITS_CHECK_CLOSE(e4.value(), 20.0);

    bu::quantity<bu::cgs::force> F0 = 20 * bu::cgs::dyne;
    BOOST_UNITS_CHECK_CLOSE(F0.value(), 20.0);

    bu::quantity<bu::si::force> F3(F0);
    BOOST_UNITS_CHECK_CLOSE(F3.value(), 2.0e-4);

    bu::quantity<bu::si::force> F5(20 * bu::cgs::dyne);
    BOOST_UNITS_CHECK_CLOSE(F5.value(), 2.0e-4);

    // same type
    BOOST_CHECK_EQUAL(boost::units::conversion_factor(si_length(), si_length()), 1.0);
}

BOOST_AUTO_TEST_CASE(test_dimensionless_conversions) {
    typedef bu::divide_typeof_helper<bu::cgs::force, bu::si::force>::type mixed_dimensionless;

    bu::quantity<bu::si::dimensionless> dimensionless_test1(1.0*bu::cgs::dyne/bu::si::newton);
    BOOST_CHECK(dimensionless_test1 == 1e-5);

    typedef bu::multiply_typeof_helper<bu::si::length, bu::cgs::length>::type m_cm;
    typedef bu::divide_typeof_helper<m_cm, m_cm>::type heterogeneous_dimensionless;
    bu::quantity<heterogeneous_dimensionless> dimensionless_test2(1.0*bu::cgs::dyne/bu::si::newton);
    BOOST_CHECK(dimensionless_test2.value() == 1e-5);
    bu::quantity<bu::divide_typeof_helper<bu::cgs::force, bu::si::force>::type> dimensionless_test3(dimensionless_test2);
    BOOST_UNITS_CHECK_CLOSE(dimensionless_test3.value(), 1.0);

    BOOST_UNITS_CHECK_CLOSE(boost::units::conversion_factor(mixed_dimensionless(), heterogeneous_dimensionless()), 1e-5);
    BOOST_UNITS_CHECK_CLOSE(boost::units::conversion_factor(heterogeneous_dimensionless(), mixed_dimensionless()), 1e5);


    //m/cm -> g/kg
    bu::quantity<bu::divide_typeof_helper<bu::si::length, bu::cgs::length>::type> dimensionless_test4(2.0 * bu::si::meters / bu::cgs::centimeters);
    bu::quantity<bu::divide_typeof_helper<bu::cgs::mass, bu::si::mass>::type> dimensionless_test5(dimensionless_test4);
    BOOST_UNITS_CHECK_CLOSE(dimensionless_test5.value(), 2e5);
}
