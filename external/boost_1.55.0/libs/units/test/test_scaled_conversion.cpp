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

#include <boost/units/quantity.hpp>
#include <boost/units/conversion.hpp>
#include <boost/units/unit.hpp>
#include <boost/units/base_unit.hpp>
#include <boost/units/scaled_base_unit.hpp>
#include <boost/units/scale.hpp>
#include <boost/units/base_dimension.hpp>

#include <boost/test/minimal.hpp>

#define BOOST_UNITS_CHECK_CLOSE(a, b) (BOOST_CHECK((std::abs((a) - (b)) < .0000001)))

namespace bu = boost::units;

struct base_dimension1 : bu::base_dimension<base_dimension1, 1> {};

struct base_unit1 : bu::base_unit<base_unit1, base_dimension1::dimension_type, 1> {};
struct base_unit2 : bu::base_unit<base_unit2, base_dimension1::dimension_type, 2> {};
struct base_unit3 : bu::base_unit<base_unit3, base_dimension1::dimension_type, 3> {};

typedef bu::scaled_base_unit<base_unit2, bu::scale<10, bu::static_rational<3> > > scaled_base_unit2;

BOOST_UNITS_DEFINE_CONVERSION_FACTOR(base_unit1, scaled_base_unit2, double, 5);
BOOST_UNITS_DEFINE_CONVERSION_FACTOR(scaled_base_unit2, base_unit3, double, 3);

int test_main(int,char *[])
{
    BOOST_UNITS_CHECK_CLOSE(bu::conversion_factor(base_unit1::unit_type(), base_unit2::unit_type()), 5000);
    BOOST_UNITS_CHECK_CLOSE(bu::conversion_factor(base_unit2::unit_type(), base_unit3::unit_type()), 0.003);
    BOOST_UNITS_CHECK_CLOSE(bu::conversion_factor(scaled_base_unit2::unit_type(), base_unit2::unit_type()), 1000);

    return(0);
}
