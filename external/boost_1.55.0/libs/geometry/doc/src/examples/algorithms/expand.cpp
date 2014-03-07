// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[expand
//` Shows the usage of expand

#include <iostream>
#include <list>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

int main()
{
    typedef boost::geometry::model::d2::point_xy<short int> point_type;
    typedef boost::geometry::model::box<point_type> box_type;

    using boost::geometry::expand;

    box_type box = boost::geometry::make_inverse<box_type>(); /*< expand is usually preceded by a call to assign_inverse or make_inverse  >*/

    expand(box, point_type(0, 0));
    expand(box, point_type(1, 2));
    expand(box, point_type(5, 4));
    expand(box, boost::geometry::make<box_type>(3, 3, 5, 5));

    std::cout << boost::geometry::dsv(box) << std::endl;

    return 0;
}

//]

//[expand_output
/*`
Output:
[pre
((0, 0), (5, 5))
]
*/
//]
