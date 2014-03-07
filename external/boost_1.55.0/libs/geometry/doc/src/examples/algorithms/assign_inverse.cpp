// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[assign_inverse
//` Usage of assign_inverse and expand to conveniently determine bounding 3D box of two points

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point.hpp>

using namespace boost::geometry;

int main()
{
    typedef model::point<float, 3, cs::cartesian> point;
    typedef model::box<point> box;

    box all;
    assign_inverse(all);
    std::cout << dsv(all) << std::endl;
    expand(all, point(0, 0, 0));
    expand(all, point(1, 2, 3));
    std::cout << dsv(all) << std::endl;

    return 0;
}

//]


//[assign_inverse_output
/*`
Output:
[pre
((3.40282e+038, 3.40282e+038, 3.40282e+038), (-3.40282e+038, -3.40282e+038, -3.40282e+038))
((0, 0, 0), (1, 2, 3))]
*/
//]
