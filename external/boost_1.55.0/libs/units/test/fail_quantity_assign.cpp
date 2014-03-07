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
    
\brief fail_quantity_assign.cpp

\details
Test assignment of quantity from different dimensions.

Output:
@verbatim
@endverbatim
**/

#include <boost/units/quantity.hpp>
#include <boost/units/systems/si.hpp>

namespace bu = boost::units;

int main(int argc,char *argv[])
{

    const bu::quantity<bu::si::length> T1;
    T1 = 2.0 * bu::si::seconds;

    return 0;
}
