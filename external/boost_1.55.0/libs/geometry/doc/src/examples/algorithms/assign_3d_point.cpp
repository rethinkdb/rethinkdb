// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[assign_3d_point
//` Use assign to set three coordinates of a 3D point

#include <iostream>
#include <iomanip>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>

int main()
{
    boost::geometry::model::point<double, 3, boost::geometry::cs::cartesian> p;
    boost::geometry::assign_values(p, 1.2345, 2.3456, 3.4567);

    std::cout << boost::geometry::dsv(p) << std::endl;

    return 0;
}

//]


//[assign_3d_point_output
/*`
Output:
[pre
(1.2345, 2.3456, 3.4567)
]
*/
//]
