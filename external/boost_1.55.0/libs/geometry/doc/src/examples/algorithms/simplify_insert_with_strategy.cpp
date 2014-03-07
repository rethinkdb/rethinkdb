// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[simplify_inserter
//` Simplify a linestring using an output iterator

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>

int main()
{
    typedef boost::geometry::model::d2::point_xy<double> P;
    typedef boost::geometry::model::linestring<P> L;

    L line;
    boost::geometry::read_wkt("linestring(1.1 1.1, 2.5 2.1, 3.1 3.1, 4.9 1.1, 3.1 1.9)", line);

    typedef boost::geometry::strategy::distance::projected_point<P, P> DS;
    typedef boost::geometry::strategy::simplify::douglas_peucker<P, DS> simplification;

    L simplified;
    boost::geometry::simplify_inserter(line, std::back_inserter(simplified), 0.5, simplification()); //std::ostream_iterator<P>(std::cout, "\n"), 0.5);//);
    //std::cout << simplified[0];
    //boost::geometry::simplify_inserter(line, std::ostream_iterator<P>(std::cout, "\n"), 0.5);//, simplification());

    std::ostream_iterator<P> out(std::cout, "\n");
    std::copy(simplified.begin(), simplified.end(), out);

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
simplify_inserter: 16
simplify_inserter: 0.339837
]
*/
//]
/*
OUTPUT
POINT(1.1 1.1)  original: ((1.1, 1.1), (2.5, 2.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
simplified: ((1.1, 1.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
*/
/*
OUTPUT
POINT(1.1 1.1)  original: ((1.1, 1.1), (2.5, 2.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
simplified: ((1.1, 1.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
*/
/*
OUTPUT
POINT(1.1 1.1)  original: ((1.1, 1.1), (2.5, 2.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
simplified: ((1.1, 1.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
*/
