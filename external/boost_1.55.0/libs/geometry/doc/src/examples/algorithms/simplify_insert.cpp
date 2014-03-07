// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[simplify_inserter
//` Simplify a linestring using a back inserter

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

int main()
{
    typedef boost::geometry::model::d2::point_xy<double> P;
    typedef boost::geometry::model::linestring<P> L;

    L line;
    line.push_back(P(1.1, 1.1));
    line.push_back(P(2.5, 2.1));
    line.push_back(P(3.1, 3.1));
    line.push_back(P(4.9, 1.1));
    line.push_back(P(3.1, 1.9));

    L simplified;
    boost::geometry::simplify_inserter(line, std::back_inserter(simplified), 0.5);

    std::cout
        << "  original: " << boost::geometry::dsv(line) << std::endl
        << "simplified: " << boost::geometry::dsv(simplified) << std::endl;

    return 0;
}

//]


//[simplify_inserter_output
/*`
Output:
[pre
  original: ((1.1, 1.1), (2.5, 2.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
simplified: ((1.1, 1.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
]
*/
//]
