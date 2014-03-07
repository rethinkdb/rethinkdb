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
    
\brief composite_output.cpp

\details An example of textual representations of units.

Output:
@verbatim

//[conversion_output_output
2 dyn
2 dyn
2 dyne
cm g s^-1
centimeter gram second^-1
dyn
dyne
n
nano
n
nano
F
farad
1 F
1 farad
nF
nanofarad
1 nF
1 nanofarad
n(cm g s^-1)
nano(centimeter gram second^-1)
//]

@endverbatim
**/
#include <boost/units/quantity.hpp>
#include <boost/units/systems/cgs.hpp>
#include <boost/units/io.hpp>
#include <boost/units/scale.hpp>

#include <boost/units/detail/utility.hpp>

#include <boost/units/systems/si/capacitance.hpp>
#include <boost/units/systems/si/io.hpp>
#include <boost/units/systems/si/prefixes.hpp>

#include <iostream>
#include <sstream>

namespace boost {

namespace units {

//[composite_output_snippet_1

std::string name_string(const cgs::force&)
{
    return "dyne";
}

std::string symbol_string(const cgs::force&)
{
    return "dyn";
}

//]

}

}

int main() 
{
    using namespace boost::units;
    using boost::units::cgs::centimeter;
    using boost::units::cgs::gram;
    using boost::units::cgs::second;
    using boost::units::cgs::dyne;
        
    //[composite_output_snippet_2]
    std::cout << 2.0 * dyne << std::endl
              << symbol_format << 2.0 * dyne << std::endl
              << name_format << 2.0 * dyne << std::endl
              << symbol_format << gram*centimeter/second << std::endl
              << name_format << gram*centimeter/second << std::endl
              << symbol_format << gram*centimeter/(second*second) << std::endl
              << name_format << gram*centimeter/(second*second) << std::endl
              << symbol_string(scale<10,static_rational<-9> >()) << std::endl
              << name_string(scale<10,static_rational<-9> >()) << std::endl
              << symbol_format << si::nano << std::endl
              << name_format << si::nano << std::endl
              << symbol_format << si::farad << std::endl
              << name_format << si::farad << std::endl
              << symbol_format << 1.0*si::farad << std::endl
              << name_format << 1.0*si::farad << std::endl
              << symbol_format << si::farad*si::nano << std::endl
              << name_format << si::farad*si::nano << std::endl
              << symbol_format << 1.0*si::farad*si::nano << std::endl
              << name_format << 1.0*si::farad*si::nano << std::endl
              << symbol_format << si::nano*gram*centimeter/second << std::endl
              << name_format << si::nano*gram*centimeter/second << std::endl;
    //]
              
    return 0;
}
