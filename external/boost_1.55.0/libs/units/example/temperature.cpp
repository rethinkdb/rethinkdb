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
    
\brief temperature.cpp

\details
Conversions between Fahrenheit and Kelvin for absolute temperatures and 
temperature differences.

Output:
@verbatim

//[ temperature_output_1
{ 32 } F
{ 273.15 } K
{ 273.15 } K
[ 32 ] F
[ 17.7778 ] K
[ 17.7778 ] K
//]

@endverbatim
**/

#include <iomanip>
#include <iostream>

#include <boost/units/absolute.hpp>
#include <boost/units/get_system.hpp>
#include <boost/units/io.hpp>
#include <boost/units/unit.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/systems/si/temperature.hpp>
#include <boost/units/detail/utility.hpp>

#include <boost/units/base_units/temperature/fahrenheit.hpp>

using namespace boost::units;

namespace boost {

namespace units {

namespace fahrenheit {

//[temperature_snippet_1
typedef temperature::fahrenheit_base_unit::unit_type    temperature;
typedef get_system<temperature>::type                   system;

BOOST_UNITS_STATIC_CONSTANT(degree,temperature);
BOOST_UNITS_STATIC_CONSTANT(degrees,temperature);
//]

} // fahrenheit

} // namespace units

} // namespace boost

int main()
{
    //[temperature_snippet_3
    quantity<absolute<fahrenheit::temperature> >    T1p(
        32.0*absolute<fahrenheit::temperature>());
    quantity<fahrenheit::temperature>               T1v(
        32.0*fahrenheit::degrees);
    
    quantity<absolute<si::temperature> >            T2p(T1p);
    quantity<si::temperature>                       T2v(T1v);
    //]

    typedef conversion_helper<
        quantity<absolute<fahrenheit::temperature> >,
        quantity<absolute<si::temperature> > >          absolute_conv_type;
    typedef conversion_helper<
        quantity<fahrenheit::temperature>,
        quantity<si::temperature> >                     relative_conv_type;
    
    std::cout << T1p << std::endl
              << absolute_conv_type::convert(T1p) << std::endl
              << T2p << std::endl
              << T1v << std::endl
              << relative_conv_type::convert(T1v) << std::endl
              << T2v << std::endl
              << std::endl;

    return 0;
}
