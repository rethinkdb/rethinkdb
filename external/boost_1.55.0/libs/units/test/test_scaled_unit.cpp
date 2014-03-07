// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** 
\file
    
\brief test_scaled_conversion.cpp

\details
Test unit scaling

Output:
@verbatim
@endverbatim
**/

#define BOOST_TEST_MAIN

#include <boost/units/systems/si/prefixes.hpp>
#include <boost/units/systems/si/time.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/io.hpp>

#include <sstream>

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

namespace bu = boost::units;
namespace si = boost::units::si;

BOOST_AUTO_TEST_CASE(test_scaled_to_plain) {
    bu::quantity<si::time> s1 = 12.5 * si::seconds;
    bu::quantity<si::time> s2(si::nano * s1);
    BOOST_CHECK_CLOSE_FRACTION(1e-9 * s1.value(), s2.value(), 0.000000001);
}

BOOST_AUTO_TEST_CASE(test_plain_to_scaled) {
    bu::quantity<si::time> s1 = 12.5 * si::seconds;
    typedef bu::multiply_typeof_helper<si::nano_type, si::time>::type time_unit;
    bu::quantity<time_unit> s2(s1);
    BOOST_CHECK_CLOSE_FRACTION(1e9 * s1.value(), s2.value(), 0.000000001);
}

BOOST_AUTO_TEST_CASE(test_scaled_to_scaled) {
    typedef bu::multiply_typeof_helper<si::mega_type, si::time>::type mega_time_unit;
    typedef bu::multiply_typeof_helper<si::micro_type, si::time>::type micro_time_unit;
    bu::quantity<mega_time_unit> s1(12.5 * si::seconds);
    bu::quantity<micro_time_unit> s2(s1);
    BOOST_CHECK_CLOSE_FRACTION(1e12 * s1.value(), s2.value(), 0.000000001);
}

BOOST_AUTO_TEST_CASE(test_conversion_factor) {
    BOOST_CHECK_CLOSE_FRACTION(conversion_factor(si::nano*si::seconds, si::seconds), 1e-9, 0.000000001);
    BOOST_CHECK_CLOSE_FRACTION(conversion_factor(si::seconds, si::nano*si::seconds), 1e9, 0.000000001);
    BOOST_CHECK_CLOSE_FRACTION(conversion_factor(si::mega*si::seconds, si::micro*si::seconds), 1e12, 0.000000001);
}

BOOST_AUTO_TEST_CASE(test_output) {
    std::stringstream stream;
    stream << si::nano * 12.5 * si::seconds;
    BOOST_CHECK_EQUAL(stream.str(), "12.5 ns");
}

BOOST_AUTO_TEST_CASE(test_output_name) {
    std::stringstream stream;
    stream << bu::name_format << si::nano * 12.5 * si::seconds;
    BOOST_CHECK_EQUAL(stream.str(), "12.5 nanosecond");
}
