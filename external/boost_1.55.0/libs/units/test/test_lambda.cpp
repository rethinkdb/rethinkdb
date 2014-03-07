// Boost.Units - A C++ library for zero-overhead dimensional analysis and
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// $Id: test_lambda.cpp 27 2008-06-16 14:50:58Z maehne $

////////////////////////////////////////////////////////////////////////
///
/// \file test_lambda.hpp
///
/// \brief Unit test for checking the usage of Boost.Units' quantity,
///        unit, and absolute types in functors created with the
///        Boost.Lambda library.
///
/// \author Torsten Maehne
/// \date   2008-06-16
///
/// This unit test contains a check for each operator action, for
/// which a specialization of Boost.Lambda's return type deduction
/// system is made in lambda.hpp, i.e., for the operators defined for
/// Boost.Units' quantity, unit, and absolute types.
///
////////////////////////////////////////////////////////////////////////

#include <boost/function.hpp>
#include <boost/units/lambda.hpp>
#include <boost/units/absolute.hpp>
#include <boost/units/systems/si/temperature.hpp>
#include "test_header.hpp"


namespace bl = boost::lambda;
namespace bu = boost::units;
namespace si = boost::units::si;


int test_main(int, char *[])
{

    ////////////////////////////////////////////////////////////////////////
    // Test for Boost.Lambda working with overloaded operators defined
    // in <boost/units/quantity.hpp>
    ////////////////////////////////////////////////////////////////////////

    bu::quantity<bu::length> lvar = 0.0 * bu::meter;

    bu::quantity<bu::dimensionless> dlvar = 3.0;

    // quantity<Unit, Y> += quantity<Unit2, YY>
    boost::function<bu::quantity<bu::length> (bu::quantity<bu::length>)>
        f = (bl::var(lvar) += bl::_1);

    lvar = 1.0 * bu::meter;
    BOOST_CHECK((f(2.0 * bu::meter) == 3.0 * bu::meter));
    BOOST_CHECK((f(6.0 * bu::meter) == 9.0 * bu::meter));

    // quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(System), Y> += quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(System), Y>
    dlvar = 4.0;
    BOOST_CHECK(((bl::var(dlvar) += bl::_1)(3.0) == 7.0));

    // quantity<Unit, Y> -= quantity<Unit2, YY>
    lvar = 3.0 * bu::meter;
    BOOST_CHECK((f(-2.0 * bu::meter) == 1.0 * bu::meter));
    BOOST_CHECK((f(6.0 * bu::meter) == 7.0 * bu::meter));

    // quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(System), Y> -= quantity<BOOST_UNITS_DIMENSIONLESS_UNIT(System), Y>
    dlvar = 4.0;
    BOOST_CHECK(((bl::var(dlvar) -= bl::_1)(3.0) == 1.0));

    // quantity<Unit, Y> *= quantity<Unit2, YY>
    dlvar = 2.0;
    BOOST_CHECK(((bl::var(dlvar) *= bl::_1)(3.0) == 6.0));

    // quantity<Unit, Y> /= quantity<Unit2, YY>
    dlvar = 6.0;
    BOOST_CHECK(((bl::var(dlvar) /= bl::_1)(3.0) == 2.0));

    // quantity<Unit, Y> *= Y
    lvar = 3.0 * bu::meter;
    BOOST_CHECK(((bl::var(lvar) *= bl::_1)(2.0) == 6.0 * bu::meter));

    // quantity<Unit, Y> /= Y
    lvar = 6.0 * bu::meter;
    BOOST_CHECK(((bl::var(lvar) /= bl::_1)(3.0) == 2.0 * bu::meter));

    // unit<Dim, System> * Y
    BOOST_CHECK(((bl::_1 * bl::_2)(bu::meter, 2.0) == 2.0 * bu::meter));
    BOOST_CHECK(((bu::meter * bl::_1)(2.0) == 2.0 * bu::meter));

    // unit<Dim, System> / Y
    BOOST_CHECK(((bl::_1 / bl::_2)(bu::meter, 0.5) == 2.0 * bu::meter));
    BOOST_CHECK(((bu::meter / bl::_1)(0.5 * bu::second) == 2.0 * bu::meter_per_second));

    // Y * unit<Dim, System>
    BOOST_CHECK(((bl::_1 * bl::_2)(2.0, bu::meter) == 2.0 * bu::meter));
    BOOST_CHECK(((bl::_1 * bu::meter)(2.0 / bu::second) == 2.0 * bu::meter_per_second));

    // Y / unit<Dim, System>
    BOOST_CHECK(((bl::_1 / bl::_2)(3.5, bu::second) == 3.5 / bu::second));
    BOOST_CHECK(((bl::_1 / bu::second)(3.5 * bu::meter) == 3.5 * bu::meter_per_second));

    // quantity<Unit, X> * X
    BOOST_CHECK(((bl::_1 * bl::_2)(2.0, 3.0 * bu::meter) == 6.0 * bu::meter));

    // X * quantity<Unit, X>
    BOOST_CHECK(((bl::_1 * bl::_2)(4.0 * bu::joule, 2.0) == 8.0 * bu::joule));

    // quantity<Unit, X> / X
    BOOST_CHECK(((bl::_1 / bl::_2)(4.0 * bu::joule, 2.0) == 2.0 * bu::joule));

    // X / quantity<Unit, X>
    BOOST_CHECK(((3.0 / bl::_1)(2.0 * bu::second) == 1.5 / bu::second));

    // unit<Dim1, System1> * quantity<Unit2, Y>
    BOOST_CHECK(((bl::_1 * bl::_2)(bu::meter, 12.0 / bu::second) == 12.0 * bu::meter_per_second));
    BOOST_CHECK(((bu::meter * bl::_1)(12.0 / bu::second) == 12.0 * bu::meter_per_second));

    // unit<Dim1, System1> / quantity<Unit2, Y>
    BOOST_CHECK(((bl::_1 / bl::_2)(bu::meter, 0.5 * bu::second) == 2.0 * bu::meter_per_second));
    BOOST_CHECK(((bu::meter / bl::_1)(0.25 * bu::second) == 4.0 * bu::meter_per_second));

    // quantity<Unit1, Y> * unit<Dim2, System2>
    BOOST_CHECK(((bl::_1 * bl::_2)(2.0 / bu::second, bu::meter) == 2.0 * bu::meter_per_second));
    BOOST_CHECK(((bl::_1 * bu::meter)(12.0 / bu::second) == 12.0 * bu::meter_per_second));

    // quantity<Unit1, Y> / unit<Dim2, System2>
    BOOST_CHECK(((bl::_1 / bl::_2)(3.5 * bu::meter, bu::second) == 3.5 * bu::meter_per_second));
    BOOST_CHECK(((bl::_1 / bu::second)(5.0 * bu::second) == 5.0));

    // +quantity<Unit, Y>
    BOOST_CHECK(((+bl::_1)(5.0 * bu::second) == 5.0 * bu::second));

    // -quantity<Unit, Y>
    BOOST_CHECK(((-bl::_1)(5.0 * bu::second) == -5.0 * bu::second));

    // quantity<Unit1, X> + quantity<Unit2, Y>
    BOOST_CHECK(((bl::_1 + bl::_2)(2.0 * bu::meter, 4.0 * bu::meter) == 6.0 * bu::meter));

    // quantity<dimensionless, X> + Y
    BOOST_CHECK(((bl::_1 + 1.0f)(bu::quantity<bu::dimensionless>(2.0)) == 3.0));

    // X + quantity<dimensionless, Y>
    BOOST_CHECK(((1.0f + bl::_1)(bu::quantity<bu::dimensionless>(1.0)) == 2.0));

    // quantity<Unit1, X> - quantity<Unit2, Y>
    BOOST_CHECK(((bl::_1 - bl::_2)(2.0 * bu::meter, 4.0 * bu::meter) == -2.0 * bu::meter));

    // quantity<dimensionless, X> - Y
    BOOST_CHECK(((bl::_1 - 2.0f)(bu::quantity<bu::dimensionless>(1.0)) == -1.0));

    // X - quantity<dimensionless, Y>
    BOOST_CHECK(((2.0f - bl::_1)(bu::quantity<bu::dimensionless>(1.0)) == 1.0));

    // quantity<Unit1, X> * quantity<Unit2, Y>
    BOOST_CHECK(((bl::_1 * bl::_2)(2.0 * bu::kilogram, 4.0 * bu::meter_per_second) == 8.0 * bu::kilogram * bu::meter_per_second));

    // quantity<Unit1, X> / quantity<Unit2, Y>
    BOOST_CHECK(((bl::_1 / bl::_2)(2.0 * bu::meter_per_second, 4.0 * bu::meter_per_second) == 0.5));

    // quantity<Unit, X> == quantity<Unit, Y>
    BOOST_CHECK(((bl::_1 == bl::_2)(2.0 * bu::meter, 2.0 * bu::meter) == true));
    BOOST_CHECK(((bl::_1 == bl::_2)(2.0 * bu::meter, 3.0 * bu::meter) == false));

    // quantity<Unit, X> != quantity<Unit, Y>
    BOOST_CHECK(((bl::_1 != bl::_2)(2.0 * bu::meter, 2.0 * bu::meter) == false));
    BOOST_CHECK(((bl::_1 != bl::_2)(2.0 * bu::meter, 3.0 * bu::meter) == true));

    // quantity<Unit, X> < quantity<Unit, Y>
    BOOST_CHECK(((bl::_1 < bl::_2)(2.0 * bu::meter, 2.0 * bu::meter) == false));
    BOOST_CHECK(((bl::_1 < bl::_2)(2.0 * bu::meter, 3.0 * bu::meter) == true));

    // quantity<Unit, X> <= quantity<Unit, Y>
    BOOST_CHECK(((bl::_1 <= bl::_2)(2.0 * bu::meter, 2.0 * bu::meter) == true));
    BOOST_CHECK(((bl::_1 <= bl::_2)(2.0 * bu::meter, 3.0 * bu::meter) == true));
    BOOST_CHECK(((bl::_1 <= bl::_2)(4.0 * bu::meter, 3.0 * bu::meter) == false));

    // quantity<Unit, X> > quantity<Unit, Y>
    BOOST_CHECK(((bl::_1 > bl::_2)(2.0 * bu::meter, 2.0 * bu::meter) == false));
    BOOST_CHECK(((bl::_1 > bl::_2)(2.0 * bu::meter, 3.0 * bu::meter) == false));
    BOOST_CHECK(((bl::_1 > bl::_2)(4.0 * bu::meter, 3.0 * bu::meter) == true));

    // quantity<Unit, X> >= quantity<Unit, Y>
    BOOST_CHECK(((bl::_1 >= bl::_2)(2.0 * bu::meter, 2.0 * bu::meter) == true));
    BOOST_CHECK(((bl::_1 >= bl::_2)(2.0 * bu::meter, 3.0 * bu::meter) == false));
    BOOST_CHECK(((bl::_1 >= bl::_2)(4.0 * bu::meter, 3.0 * bu::meter) == true));


    ////////////////////////////////////////////////////////////////////////
    // Test for Boost.Lambda working with overloaded operators defined
    // in <boost/units/unit.hpp>
    ////////////////////////////////////////////////////////////////////////

    // +unit<Dim, System>
    BOOST_CHECK(((+bl::_1)(bu::meter) == bu::meter));

    // -unit<Dim, System>
    BOOST_CHECK(((-bl::_1)(bu::meter) == bu::meter));

    // unit<Dim1, System1> + unit<Dim2, System2>
    BOOST_CHECK(((bl::_1 + bu::meter)(bu::meter) == bu::meter));
    BOOST_CHECK(((bu::meter + bl::_1)(bu::meter) == bu::meter));
    BOOST_CHECK(((bl::_1 + bl::_2)(bu::meter, bu::meter) == bu::meter));

    // unit<Dim1, System1> - unit<Dim2, System2>
    BOOST_CHECK(((bl::_1 - bl::_2)(bu::meter, bu::meter) == bu::meter));
    BOOST_CHECK(((bl::_1 - bu::meter)(bu::meter) == bu::meter));
    BOOST_CHECK(((bu::meter - bl::_1)(bu::meter) == bu::meter));

    // unit<Dim1, System1> * unit<Dim2, System2>
    BOOST_CHECK(((bl::_1 * bl::_2)(bu::meter, bu::meter) == bu::meter * bu::meter));
    BOOST_CHECK(((bl::_1 * bu::meter)(bu::meter) == bu::meter * bu::meter));

    // unit<Dim1, System1> / unit<Dim2, System2>
    BOOST_CHECK(((bl::_1 / bl::_2)(bu::meter, bu::second) == bu::meter_per_second));
    BOOST_CHECK(((bl::_1 / bu::second)(bu::meter) == bu::meter_per_second));

    // unit<Dim1, System1> == unit<Dim2, System2>
    BOOST_CHECK(((bl::_1 == bu::meter)(bu::meter) == true));
    BOOST_CHECK(((bl::_1 == bu::meter)(bu::second) == false));

    // unit<Dim1, System1> != unit<Dim2, System2>
    BOOST_CHECK(((bl::_1 != bu::meter)(bu::meter) == false));
    BOOST_CHECK(((bl::_1 != bu::meter)(bu::second) == true));


    ////////////////////////////////////////////////////////////////////////
    // Test for Boost.Lambda working with overloaded operators defined
    // in <boost/units/absolute.hpp>
    ////////////////////////////////////////////////////////////////////////

    // absolute<Y> += Y
    bu::quantity<bu::absolute<si::temperature> > Ta = 270.0 * bu::absolute<si::temperature>();
    (Ta += bl::_1)(30.0 * si::kelvin);
    BOOST_CHECK(( Ta == 300.0 * bu::absolute<si::temperature>()));

    // absolute<Y> -= Y
    Ta = 270 * bu::absolute<si::temperature>();
    (Ta -= bl::_1)(-30.0 * si::kelvin);
    BOOST_CHECK(( Ta == 300.0 * bu::absolute<si::temperature>()));

    // absolute<Y> + Y
    BOOST_CHECK(((270.0 * bu::absolute<si::temperature>() + bl::_1)(30.0 * si::kelvin) == 300.0 * bu::absolute<si::temperature>()));

    // Y + absolute<Y>
    BOOST_CHECK(((bl::_1 + 270.0 * bu::absolute<si::temperature>())(30.0 * si::kelvin) == 300.0 * bu::absolute<si::temperature>()));

    // absolute<Y> - Y
    BOOST_CHECK(((270.0 * bu::absolute<si::temperature>() - bl::_1)(30.0 * si::kelvin) == 240.0 * bu::absolute<si::temperature>()));

    // absolute<Y> - absolute<Y>
    BOOST_CHECK(((bl::_1 - 270.0 * bu::absolute<si::temperature>())(300.0 * bu::absolute<si::temperature>()) == 30.0 * si::kelvin));

    // T * absolute<unit<D, S> >
    BOOST_CHECK(((bl::_1 * bu::absolute<si::temperature>())(300.0) == 300.0 * bu::absolute<si::temperature>()));
    BOOST_CHECK(((bl::_1 * bl::_2)(300.0, bu::absolute<si::temperature>()) == 300.0 * bu::absolute<si::temperature>()));

    // absolute<unit<D, S> > * T
    BOOST_CHECK(((bu::absolute<si::temperature>() * bl::_1)(300.0) == 300.0 * bu::absolute<si::temperature>()));
    BOOST_CHECK(((bl::_1 * bl::_2)(bu::absolute<si::temperature>(), 300.0) == 300.0 * bu::absolute<si::temperature>()));


    return 0;
}
