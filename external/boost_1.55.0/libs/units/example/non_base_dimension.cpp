// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** 
\file
    
\brief non_base_dimension.cpp

\details
Another example of user-defined units with conversions.

Output:
@verbatim

//[non_base_dimension_output
//]

@endverbatim
**/

#include <iostream>

#include <boost/units/io.hpp>
#include <boost/units/conversion.hpp>
#include <boost/units/unit.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/physical_dimensions.hpp>
#include <boost/units/base_unit.hpp>
#include <boost/units/make_system.hpp>

namespace boost {

namespace units {

//[non_base_dimension_snippet_1

struct imperial_gallon_tag :
    base_unit<imperial_gallon_tag, volume_dimension, 1> { };

typedef make_system<imperial_gallon_tag>::type imperial;

typedef unit<volume_dimension,imperial> imperial_gallon;

struct us_gallon_tag : base_unit<us_gallon_tag, volume_dimension, 2> { };

typedef make_system<us_gallon_tag>::type us;

typedef unit<volume_dimension,us> us_gallon;

//]

} // namespace units

} // namespace boost

BOOST_UNITS_DEFINE_CONVERSION_FACTOR(boost::units::imperial_gallon_tag,
                                     boost::units::us_gallon_tag,
                                     double, 1.2009499255);

using namespace boost::units;

int main(void)
{
    quantity<imperial_gallon>   ig1(1.0*imperial_gallon());
    quantity<us_gallon>         ug1(1.0*us_gallon());
    
    quantity<imperial_gallon>   ig2(ug1);
    quantity<us_gallon>         ug2(ig1);
    
    return 0;
}
