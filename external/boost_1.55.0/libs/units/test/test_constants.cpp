// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2009 Matthias Christian Schabel
// Copyright (C) 2007-2009 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** 
\file
    
\brief test_constants.cpp

\details
Test all combinations of operators with the constants.

**/

#include <boost/units/systems/detail/constants.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/pow.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/time.hpp>

using boost::units::quantity;
using boost::units::si::length;
using boost::units::si::meters;
using boost::units::si::seconds;
using boost::units::static_rational;
using boost::units::pow;
using boost::units::root;

BOOST_UNITS_PHYSICAL_CONSTANT(length_constant, quantity<length>, 2.0 * meters, 0.5 * meters);

template<class T>
void check_same(const T&, const T&);

template<class T>
typename T::value_type unwrap(const boost::units::constant<T>&);

template<class T>
T unwrap(const T&);

#define BOOST_UNITS_CHECK_RESULT(arg1, op, arg2) check_same((arg1) op (arg2), unwrap(arg1) op unwrap(arg2));

void test_add() {
    BOOST_UNITS_CHECK_RESULT(length_constant, +, length_constant);
    BOOST_UNITS_CHECK_RESULT(length_constant, +, 1.0 * meters);
    BOOST_UNITS_CHECK_RESULT(1.0* meters, +, length_constant);
}

void test_subtract() {
    BOOST_UNITS_CHECK_RESULT(length_constant, -, length_constant);
    BOOST_UNITS_CHECK_RESULT(length_constant, -, 1.0 * meters);
    BOOST_UNITS_CHECK_RESULT(1.0* meters, -, length_constant);
}

void test_multiply() {
    BOOST_UNITS_CHECK_RESULT(length_constant, *, length_constant);
    BOOST_UNITS_CHECK_RESULT(length_constant, *, 1.0 * seconds);
    BOOST_UNITS_CHECK_RESULT(1.0 * seconds, *, length_constant);
    BOOST_UNITS_CHECK_RESULT(length_constant, *, 1.0);
    BOOST_UNITS_CHECK_RESULT(1.0, *, length_constant);
    BOOST_UNITS_CHECK_RESULT(length_constant, *, seconds);
    BOOST_UNITS_CHECK_RESULT(seconds, *, length_constant);
}

void test_divide() {
    BOOST_UNITS_CHECK_RESULT(length_constant, /, length_constant);
    BOOST_UNITS_CHECK_RESULT(length_constant, /, 1.0 * seconds);
    BOOST_UNITS_CHECK_RESULT(1.0 * seconds, /, length_constant);
    BOOST_UNITS_CHECK_RESULT(length_constant, /, 1.0);
    BOOST_UNITS_CHECK_RESULT(1.0, /, length_constant);
    BOOST_UNITS_CHECK_RESULT(length_constant, /, seconds);
    BOOST_UNITS_CHECK_RESULT(seconds, /, length_constant);
}

void test_pow() {
    check_same(pow<2>(length_constant), pow<2>(unwrap(length_constant)));
    check_same(root<2>(length_constant), root<2>(unwrap(length_constant)));
    check_same(pow<5>(length_constant), pow<5>(unwrap(length_constant)));
    check_same(root<5>(length_constant), root<5>(unwrap(length_constant)));
    check_same(pow<static_rational<2, 3> >(length_constant), pow<static_rational<2, 3> >(unwrap(length_constant)));
    check_same(root<static_rational<2, 3> >(length_constant), root<static_rational<2, 3> >(unwrap(length_constant)));
}
