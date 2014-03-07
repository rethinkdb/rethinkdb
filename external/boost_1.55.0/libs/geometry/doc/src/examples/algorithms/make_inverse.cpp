// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[make_inverse
//` Usage of make_inverse and expand to conveniently determine bounding box of several objects

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

using namespace boost::geometry;

int main()
{

    typedef model::d2::point_xy<double> point;
    typedef model::box<point> box;

    box all = make_inverse<box>();
    std::cout << dsv(all) << std::endl;
    expand(all, make<box>(0, 0, 3, 4));
    expand(all, make<box>(2, 2, 5, 6));
    std::cout << dsv(all) << std::endl;

    return 0;
}

//]


//[make_inverse_output
/*`
Output:
[pre
((1.79769e+308, 1.79769e+308), (-1.79769e+308, -1.79769e+308))
((0, 0), (5, 6))
]
*/
//]
