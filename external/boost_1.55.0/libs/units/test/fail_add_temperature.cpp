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
    
\brief fail_add_temperature.cpp

\details
Verify that adding two absolute temeratures fails miserably.

Output:
@verbatim
@endverbatim
**/

#include <boost/units/quantity.hpp>
#include <boost/units/absolute.hpp>
#include <boost/units/systems/si/temperature.hpp>

namespace bu = boost::units;

int main(int,char *[])
{

    bu::quantity<bu::absolute<bu::si::temperature> > q(2.0 * bu::absolute<bu::si::temperature>());

    q += q;

    return 0;
}
