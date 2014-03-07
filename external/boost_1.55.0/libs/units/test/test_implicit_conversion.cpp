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
    
\brief test_implicit_conversion.cpp

\details
Test implicit conversions for quantity.

Output:
@verbatim
@endverbatim
**/

#include <boost/test/minimal.hpp>

#include <boost/units/static_constant.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/systems/cgs.hpp>

namespace bu = boost::units;

int test_main(int,char *[])
{
    //// si->si always true
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::length_base_dimension,bu::si::system_tag,bu::si::system_tag>::value == true));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::mass_base_dimension,bu::si::system_tag,bu::si::system_tag>::value == true));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::time_base_dimension,bu::si::system_tag,bu::si::system_tag>::value == true));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::current_base_dimension,bu::si::system_tag,bu::si::system_tag>::value == true));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::temperature_base_dimension,bu::si::system_tag,bu::si::system_tag>::value == true));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::amount_base_dimension,bu::si::system_tag,bu::si::system_tag>::value == true));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::luminous_intensity_base_dimension,bu::si::system_tag,bu::si::system_tag>::value == true));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::plane_angle_base_dimension,bu::si::system_tag,bu::si::system_tag>::value == true));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::solid_angle_base_dimension,bu::si::system_tag,bu::si::system_tag>::value == true));

    //// cgs->cgs always true
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::length_base_dimension,bu::cgs::system_tag,bu::cgs::system_tag>::value == true));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::mass_base_dimension,bu::cgs::system_tag,bu::cgs::system_tag>::value == true));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::time_base_dimension,bu::cgs::system_tag,bu::cgs::system_tag>::value == true));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::current_base_dimension,bu::cgs::system_tag,bu::cgs::system_tag>::value == true));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::temperature_base_dimension,bu::cgs::system_tag,bu::cgs::system_tag>::value == true));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::amount_base_dimension,bu::cgs::system_tag,bu::cgs::system_tag>::value == true));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::luminous_intensity_base_dimension,bu::cgs::system_tag,bu::cgs::system_tag>::value == true));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::plane_angle_base_dimension,bu::cgs::system_tag,bu::cgs::system_tag>::value == true));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::solid_angle_base_dimension,bu::cgs::system_tag,bu::cgs::system_tag>::value == true));

    //// si->cgs
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::length_base_dimension,bu::si::system_tag,bu::cgs::system_tag>::value == false));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::mass_base_dimension,bu::si::system_tag,bu::cgs::system_tag>::value == false));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::time_base_dimension,bu::si::system_tag,bu::cgs::system_tag>::value == true));
    
    BOOST_CHECK((bu::is_implicitly_convertible<bu::si::length,bu::cgs::length>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::si::mass,bu::cgs::mass>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::si::time,bu::cgs::time>::value == true));

    BOOST_CHECK((bu::is_implicitly_convertible<bu::si::acceleration,bu::cgs::acceleration>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::si::area,bu::cgs::area>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::si::energy,bu::cgs::energy>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::si::force,bu::cgs::force>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::si::frequency,bu::cgs::frequency>::value == true));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::si::mass_density,bu::cgs::mass_density>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::si::momentum,bu::cgs::momentum>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::si::power,bu::cgs::power>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::si::pressure,bu::cgs::pressure>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::si::velocity,bu::cgs::velocity>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::si::wavenumber,bu::cgs::wavenumber>::value == false));
    
    //// cgs->si
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::length_base_dimension,bu::cgs::system_tag,bu::si::system_tag>::value == false));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::mass_base_dimension,bu::cgs::system_tag,bu::si::system_tag>::value == false));
    //BOOST_CHECK((bu::base_unit_is_implicitly_convertible<bu::time_base_dimension,bu::cgs::system_tag,bu::si::system_tag>::value == true));
              
    BOOST_CHECK((bu::is_implicitly_convertible<bu::cgs::length,bu::si::length>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::cgs::mass,bu::si::mass>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::cgs::time,bu::si::time>::value == true));

    BOOST_CHECK((bu::is_implicitly_convertible<bu::cgs::acceleration,bu::si::acceleration>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::cgs::area,bu::si::area>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::cgs::energy,bu::si::energy>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::cgs::force,bu::si::force>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::cgs::frequency,bu::si::frequency>::value == true));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::cgs::mass_density,bu::si::mass_density>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::cgs::momentum,bu::si::momentum>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::cgs::power,bu::si::power>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::cgs::pressure,bu::si::pressure>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::cgs::velocity,bu::si::velocity>::value == false));
    BOOST_CHECK((bu::is_implicitly_convertible<bu::cgs::wavenumber,bu::si::wavenumber>::value == false));
    
    const bu::quantity<bu::si::time>    S1(2.0*bu::si::seconds);
    const bu::quantity<bu::cgs::time>   S2 = S1;

    BOOST_CHECK((S1.value() == S2.value()));
    
    const bu::quantity<bu::si::catalytic_activity>  S3(2.0*bu::si::catalytic_activity());
    
    
    const bu::quantity<bu::cgs::time>   C1(2.0*bu::cgs::seconds);
    const bu::quantity<bu::si::time>    C2 = C1;

    BOOST_CHECK((C1.value() == C2.value()));

    return 0;
}
