// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[register_box_templated
//` Show the use of the macro BOOST_GEOMETRY_REGISTER_BOX_TEMPLATED

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/register/box.hpp>

template <typename P>
struct my_box
{
    P ll, ur;
};

// Register the box type
BOOST_GEOMETRY_REGISTER_BOX_TEMPLATED(my_box, ll, ur)

int main()
{
    typedef my_box<boost::geometry::model::d2::point_xy<double> > box;
    box b = boost::geometry::make<box>(0, 0, 2, 2);
    std::cout << "Area: "  << boost::geometry::area(b) << std::endl;
    return 0;
}

//]


//[register_box_templated_output
/*`
Output:
[pre
Area: 4
]
*/
//]
