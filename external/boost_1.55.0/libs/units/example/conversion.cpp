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
    
\brief conversion.cpp

\details
Test explicit and implicit unit conversion.

Output:
@verbatim

//[conversion_output_1
L1 = 2 m
L2 = 2 m
L3 = 2 m
L4 = 200 cm
L5 = 5 m
L6 = 4 m
L7 = 200 cm
//]

//[conversion_output_2
volume (m^3)  = 1 m^3
volume (cm^3) = 1e+06 cm^3
volume (m^3)  = 1 m^3

energy (joules) = 1 J
energy (ergs)   = 1e+07 erg
energy (joules) = 1 J

velocity (2 m/s)  = 2 m s^-1
velocity (2 cm/s) = 0.02 m s^-1
//]

@endverbatim
**/

#include <iostream>

#include <boost/units/io.hpp>
#include <boost/units/pow.hpp>
#include <boost/units/systems/cgs.hpp>
#include <boost/units/systems/cgs/io.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/systems/si/io.hpp>

using namespace boost::units;

int main()
{
    // test quantity_cast
    {
    // implicit value_type conversions
    //[conversion_snippet_1
    quantity<si::length>     L1 = quantity<si::length,int>(int(2.5)*si::meters);
    quantity<si::length,int> L2(quantity<si::length,double>(2.5*si::meters));
    //]
    
    //[conversion_snippet_3
    quantity<si::length,int> L3 = static_cast<quantity<si::length,int> >(L1);
    //]
    
    //[conversion_snippet_4
    quantity<cgs::length>    L4 = static_cast<quantity<cgs::length> >(L1);
    //]
    
    quantity<si::length,int> L5(4*si::meters),
                             L6(5*si::meters);
    quantity<cgs::length>    L7(L1);
    
    swap(L5,L6);
    
    std::cout << "L1 = " << L1 << std::endl
              << "L2 = " << L2 << std::endl
              << "L3 = " << L3 << std::endl
              << "L4 = " << L4 << std::endl
              << "L5 = " << L5 << std::endl
              << "L6 = " << L6 << std::endl
              << "L7 = " << L7 << std::endl
              << std::endl;
    }
    
    // test explicit unit system conversion
    {
    //[conversion_snippet_5
    quantity<si::volume>    vs(1.0*pow<3>(si::meter));      
    quantity<cgs::volume>   vc(vs);
    quantity<si::volume>    vs2(vc);
                        
    quantity<si::energy>    es(1.0*si::joule);      
    quantity<cgs::energy>   ec(es);
    quantity<si::energy>    es2(ec);
                        
    quantity<si::velocity>  v1 = 2.0*si::meters/si::second,     
                            v2(2.0*cgs::centimeters/cgs::second);
    //]
    
    std::cout << "volume (m^3)  = " << vs << std::endl
              << "volume (cm^3) = " << vc << std::endl
              << "volume (m^3)  = " << vs2 << std::endl
              << std::endl;
            
    std::cout << "energy (joules) = " << es << std::endl
              << "energy (ergs)   = " << ec << std::endl
              << "energy (joules) = " << es2 << std::endl
              << std::endl;
            
    std::cout << "velocity (2 m/s)  = " << v1 << std::endl
              << "velocity (2 cm/s) = " << v2 << std::endl
              << std::endl;
    }

    return 0;
}
