// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/minimal.hpp>

#include <boost/units/base_dimension.hpp>
#include <boost/units/base_unit.hpp>
#include <boost/units/make_system.hpp>
#include <boost/units/scaled_base_unit.hpp>
#include <boost/units/conversion.hpp>
#include <boost/units/unit.hpp>

struct dimension1_tag : boost::units::base_dimension<dimension1_tag, 1> {};
struct dimension2_tag : boost::units::base_dimension<dimension2_tag, 2> {};

typedef dimension1_tag::dimension_type dimension1;
typedef dimension2_tag::dimension_type dimension2;
typedef boost::mpl::times<dimension1, dimension2>::type dimension12;

struct unit1_tag : boost::units::base_unit<unit1_tag, dimension1, 1> {}; 
struct unit2_tag : boost::units::base_unit<unit2_tag, dimension1, 2> {};
struct unit3_tag : boost::units::base_unit<unit3_tag, dimension1, 3> {};
struct unit4_tag : boost::units::base_unit<unit4_tag, dimension2, 4> {};
struct unit5_tag : boost::units::base_unit<unit5_tag, dimension12, 5> {};
struct unit6_tag : boost::units::base_unit<unit6_tag, dimension1, 6> {};
struct unit7_tag : boost::units::base_unit<unit7_tag, dimension1, 7> {};

BOOST_UNITS_DEFINE_CONVERSION_FACTOR(unit1_tag, unit2_tag, double, 2.0);
BOOST_UNITS_DEFINE_CONVERSION_FACTOR(unit2_tag, unit3_tag, double, 3.0);
BOOST_UNITS_DEFINE_CONVERSION_FACTOR(unit6_tag, unit3_tag, double, 5.0);
BOOST_UNITS_DEFINE_CONVERSION_FACTOR(unit7_tag, unit1_tag, double, 7.0);

typedef boost::units::multiply_typeof_helper<unit3_tag::unit_type, unit4_tag::unit_type>::type unit34_type;
BOOST_UNITS_DEFINE_CONVERSION_FACTOR(unit5_tag, unit34_type , double, 5.0);

template<class BaseUnit>
struct make_unit {
    typedef boost::units::unit<
        typename BaseUnit::dimension_type,
        typename boost::units::make_system<BaseUnit>::type> type;
};

BOOST_UNITS_DEFAULT_CONVERSION(unit1_tag, unit2_tag);
BOOST_UNITS_DEFAULT_CONVERSION(unit3_tag, unit2_tag);
BOOST_UNITS_DEFAULT_CONVERSION(unit5_tag, unit34_type);
BOOST_UNITS_DEFAULT_CONVERSION(unit6_tag, make_unit<unit3_tag>::type);
BOOST_UNITS_DEFAULT_CONVERSION(unit7_tag, make_unit<unit1_tag>::type);

int test_main(int, char*[]) {
    double value1 = boost::units::conversion_factor(unit3_tag::unit_type(), unit1_tag::unit_type());
    BOOST_CHECK(std::abs(value1 - 1.0/6.0) < .0000000001);
    double value2 = boost::units::conversion_factor(unit5_tag::unit_type() / unit4_tag::unit_type(), unit1_tag::unit_type());
    BOOST_CHECK(std::abs(value2 - 5.0/6.0) < .0000000001);
    typedef boost::units::scaled_base_unit<unit5_tag, boost::units::scale<2, boost::units::static_rational<1> > > scaled_unit5_tag;
    double value3 = boost::units::conversion_factor(scaled_unit5_tag::unit_type() / unit4_tag::unit_type(), unit1_tag::unit_type());
    BOOST_CHECK(std::abs(value3 - 10.0/6.0) < .0000000001);

    // check homogeneous unit conversions
    double value4 = boost::units::conversion_factor(make_unit<unit3_tag>::type(), make_unit<unit1_tag>::type());
    BOOST_CHECK(std::abs(value4 - 1.0/6.0) < .0000000001);
    double value5 = boost::units::conversion_factor(unit3_tag::unit_type(), make_unit<unit1_tag>::type());
    BOOST_CHECK(std::abs(value5 - 1.0/6.0) < .0000000001);
    double value6 = boost::units::conversion_factor(make_unit<unit3_tag>::type(), unit1_tag::unit_type());
    BOOST_CHECK(std::abs(value6 - 1.0/6.0) < .0000000001);

    // check chained homogeneous conversions
    double value7 = boost::units::conversion_factor(unit6_tag::unit_type(), unit7_tag::unit_type());
    BOOST_CHECK(std::abs(value7 - 5.0/42.0) < .0000000001);

    return(0);
}
