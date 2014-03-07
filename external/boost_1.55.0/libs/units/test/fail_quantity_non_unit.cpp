// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** 
\file
    
\brief fail_quantity_non_unit.cpp

\details

Make sure that trying to use a base_unit as though
it were a unit fails.

Output:
@verbatim
@endverbatim
**/

#include <boost/units/quantity.hpp>
#include <boost/units/base_units/si/meter.hpp>

namespace bu = boost::units;

int main(int,char *[])
{

    bu::quantity<bu::si::meter_base_unit> q;
    

    return 0;
}
