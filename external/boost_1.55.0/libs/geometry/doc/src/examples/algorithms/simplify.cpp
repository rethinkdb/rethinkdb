// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[simplify
//` Example showing how to simplify a linestring

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

/*< For this example we use Boost.Assign to add points >*/
#include <boost/assign.hpp>

using namespace boost::assign;


int main()
{
    typedef boost::geometry::model::d2::point_xy<double> xy;

    boost::geometry::model::linestring<xy> line;
    line += xy(1.1, 1.1), xy(2.5, 2.1), xy(3.1, 3.1), xy(4.9, 1.1), xy(3.1, 1.9); /*< With Boost.Assign >*/

    // Simplify it, using distance of 0.5 units
    boost::geometry::model::linestring<xy> simplified;
    boost::geometry::simplify(line, simplified, 0.5);
    std::cout
        << "  original: " << boost::geometry::dsv(line) << std::endl
        << "simplified: " << boost::geometry::dsv(simplified) << std::endl;


    return 0;
}

//]


//[simplify_output
/*`
Output:
[pre
  original: ((1.1, 1.1), (2.5, 2.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
simplified: ((1.1, 1.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
]
*/
//]
