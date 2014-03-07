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
    
\brief unit.cpp

\details
Test unit algebra.

Output:
@verbatim

//[unit_output
L             = m
L+L           = m
L-L           = m
L/L           = dimensionless 
meter*meter   = m^2
M*(L/T)*(L/T) = m^2 kg s^-2
M*(L/T)^2     = m^2 kg s^-2
L^3           = m^3
L^(3/2)       = m^(3/2)
2vM           = kg^(1/2)
(3/2)vM       = kg^(2/3)
//]

@endverbatim
**/

#include <iostream>

#include "test_system.hpp"

#include <boost/units/pow.hpp>

int main()
{
    using namespace boost::units;
    using namespace boost::units::test;

    //[unit_snippet_1
    const length                    L;
    const mass                      M;
    // needs to be namespace-qualified because of global time definition
    const boost::units::test::time  T;
    const energy                    E;
    //]
    
    std::cout << "L             = " << L << std::endl
              << "L+L           = " << L+L << std::endl
              << "L-L           = " << L-L << std::endl
              << "L/L           = " << L/L << std::endl
              << "meter*meter   = " << meter*meter << std::endl
              << "M*(L/T)*(L/T) = " << M*(L/T)*(L/T) << std::endl
              << "M*(L/T)^2     = " << M*pow<2>(L/T) << std::endl
              << "L^3           = " << pow<3>(L) << std::endl
              << "L^(3/2)       = " << pow<static_rational<3,2> >(L)
              << std::endl
              << "2vM           = " << root<2>(M) << std::endl
              << "(3/2)vM       = " << root<static_rational<3,2> >(M)
              << std::endl;

    return 0;
}
