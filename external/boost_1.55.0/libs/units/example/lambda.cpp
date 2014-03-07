// Boost.Units - A C++ library for zero-overhead dimensional analysis and
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// $Id: lambda.cpp 27 2008-06-16 14:50:58Z maehne $

////////////////////////////////////////////////////////////////////////
///
/// \file lambda.cpp
///
/// \brief Example demonstrating the usage of Boost.Units' quantity,
///        unit, and absolute types in functors created with the
///        Boost.Lambda library and stored in Boost.Function objects.
///
/// \author Torsten Maehne
/// \date   2008-06-04
///
/// A mechanical, electrical, geometrical, and thermal example
/// demonstrate how to use Boost.Units' quantity, unit, and absolute
/// types in lambda expressions. The resulting functors can be stored
/// in boost::function objects. It is also shown how to work around a
/// limitation of Boost.Lambda's bind() to help it to find the correct
/// overloaded function by specifying its signature with a
/// static_cast.
///
////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <boost/function.hpp>
#include <boost/units/io.hpp>
#include <boost/units/cmath.hpp>
#include <boost/units/pow.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/absolute.hpp>

// Include boost/units/lambda.hpp instead of boost/lambda/lambda.hpp
// for a convenient usage of Boost.Units' quantity, unit, and absolute
// types in lambda expressions. The header augments Boost.Lambda's
// return type detuction system to recognize the new types so that not
// for each arithmetic operation the return type needs to be
// explicitely specified.
#include <boost/units/lambda.hpp>

#include <boost/lambda/bind.hpp>

static const double pi = 3.14159265358979323846;

//[lambda_snippet_1

int main(int argc, char **argv) {

   using namespace std;
   namespace bl = boost::lambda;
   namespace bu = boost::units;
   namespace si = boost::units::si;


   ////////////////////////////////////////////////////////////////////////
   // Mechanical example: linear accelerated movement
   ////////////////////////////////////////////////////////////////////////

   // Initial condition variables for acceleration, speed, and displacement
   bu::quantity<si::acceleration> a = 2.0 * si::meters_per_second_squared;
   bu::quantity<si::velocity> v = 1.0 * si::meters_per_second;
   bu::quantity<si::length> s0 = 0.5 * si::meter;

   // Displacement over time
   boost::function<bu::quantity<si::length> (bu::quantity<si::time>) >
       s = 0.5 * bl::var(a) * bl::_1 * bl::_1
           + bl::var(v) * bl::_1
           + bl::var(s0);

   cout << "Linear accelerated movement:" << endl
        << "a = " << a << ", v = " << v << ", s0 = " << s0 << endl
        << "s(1.0 * si::second) = " << s(1.0 * si::second) << endl
        << endl;

   // Change initial conditions
   a = 1.0 * si::meters_per_second_squared;
   v = 2.0 * si::meters_per_second;
   s0 = -1.5 * si::meter;

   cout << "a = " << a << ", v = " << v << ", s0 = " << s0 << endl
        << "s(1.0 * si::second) = " << s(1.0 * si::second) << endl
        << endl;


   ////////////////////////////////////////////////////////////////////////
   // Electrical example: oscillating current
   ////////////////////////////////////////////////////////////////////////

   // Constants for the current amplitude, frequency, and offset current
   const bu::quantity<si::current> iamp = 1.5 * si::ampere;
   const bu::quantity<si::frequency> f = 1.0e3 * si::hertz;
   const bu::quantity<si::current> i0 = 0.5 * si::ampere;

   // The invocation of the sin function needs to be postponed using
   // bind to specify the oscillation function. A lengthy static_cast
   // to the function pointer referencing boost::units::sin() is needed
   // to avoid an "unresolved overloaded function type" error.
   boost::function<bu::quantity<si::current> (bu::quantity<si::time>) >
       i = iamp
           * bl::bind(static_cast<bu::dimensionless_quantity<si::system, double>::type (*)(const bu::quantity<si::plane_angle>&)>(bu::sin),
                      2.0 * pi * si::radian * f * bl::_1)
           + i0;

   cout << "Oscillating current:" << endl
        << "iamp = " << iamp << ", f = " << f << ", i0 = " << i0 << endl
        << "i(1.25e-3 * si::second) = " << i(1.25e-3 * si::second) << endl
        << endl;


   ////////////////////////////////////////////////////////////////////////
   // Geometric example: area calculation for a square
   ////////////////////////////////////////////////////////////////////////

   // Length constant
   const bu::quantity<si::length> l = 1.5 * si::meter;

   // Again an ugly static_cast is needed to bind pow<2> to the first
   // function argument.
   boost::function<bu::quantity<si::area> (bu::quantity<si::length>) >
       A = bl::bind(static_cast<bu::quantity<si::area> (*)(const bu::quantity<si::length>&)>(bu::pow<2>),
                    bl::_1);

   cout << "Area of a square:" << endl
        << "A(" << l <<") = " << A(l) << endl << endl;


   ////////////////////////////////////////////////////////////////////////
   // Thermal example: temperature difference of two absolute temperatures
   ////////////////////////////////////////////////////////////////////////

   // Absolute temperature constants
   const bu::quantity<bu::absolute<si::temperature> >
       Tref = 273.15 * bu::absolute<si::temperature>();
   const bu::quantity<bu::absolute<si::temperature> >
       Tamb = 300.00 * bu::absolute<si::temperature>();

   boost::function<bu::quantity<si::temperature> (bu::quantity<bu::absolute<si::temperature> >,
                                                  bu::quantity<bu::absolute<si::temperature> >)>
       dT = bl::_2 - bl::_1;

   cout << "Temperature difference of two absolute temperatures:" << endl
        << "dT(" << Tref << ", " << Tamb << ") = " << dT(Tref, Tamb) << endl
        << endl;


   return 0;
}
//]
