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
    
\brief heterogeneous_unit.cpp

\details
Test heterogeneous units and quantities.

Output:
@verbatim

//[heterogeneous_unit_output_1
1.5 m
1 g
1.5 m g
1.5 m g^-1

1 N
1 kg s^-2

1 cm kg s^-2
1 cm m^-1 kg s^-2
//]

//[heterogeneous_unit_output_2
1.5 cm m
0.015 m^2
//]

@endverbatim
**/

#define MCS_USE_DEMANGLING
//#define MCS_USE_BOOST_REGEX_DEMANGLING

#include <iostream>

#include <boost/units/io.hpp>
#include <boost/units/pow.hpp>
#include <boost/units/detail/utility.hpp>
#include <boost/units/systems/cgs.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/systems/si/io.hpp>

using namespace boost::units;

int main()
{
    //[heterogeneous_unit_snippet_1
    quantity<si::length>        L(1.5*si::meter);
    quantity<cgs::mass>         M(1.0*cgs::gram);
    
    std::cout << L << std::endl
              << M << std::endl
              << L*M << std::endl
              << L/M << std::endl
              << std::endl;
              
    std::cout << 1.0*si::meter*si::kilogram/pow<2>(si::second) << std::endl
              << 1.0*si::meter*si::kilogram/pow<2>(si::second)/si::meter
              << std::endl << std::endl;

    std::cout << 1.0*cgs::centimeter*si::kilogram/pow<2>(si::second) << std::endl
              << 1.0*cgs::centimeter*si::kilogram/pow<2>(si::second)/si::meter
              << std::endl << std::endl;
    //]
    
    //[heterogeneous_unit_snippet_2
    quantity<si::area>      A(1.5*si::meter*cgs::centimeter);
    
    std::cout << 1.5*si::meter*cgs::centimeter << std::endl
              << A << std::endl
              << std::endl;
    //]

    return 0;
}
