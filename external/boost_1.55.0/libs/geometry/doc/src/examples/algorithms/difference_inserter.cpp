// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[difference_inserter
//` Shows how the difference_inserter function can be used

#include <iostream>
#include <vector>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>

#include <boost/foreach.hpp>
/*<-*/ #include "create_svg_overlay.hpp" /*->*/

int main()
{
    typedef boost::geometry::model::polygon<boost::geometry::model::d2::point_xy<double> > polygon;

    polygon green, blue;

    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", green);

    boost::geometry::read_wkt(
        "POLYGON((4.0 -0.5 , 3.5 1.0 , 2.0 1.5 , 3.5 2.0 , 4.0 3.5 , 4.5 2.0 , 6.0 1.5 , 4.5 1.0 , 4.0 -0.5))", blue);

    std::vector<polygon> output;

    // Note that this sample simulates the symmetric difference,
    // which is also available as a separate algorithm.
    // It chains the output iterator returned by the function to the second instance.
    boost::geometry::difference_inserter<polygon>
        (
            green, blue,
                boost::geometry::difference_inserter<polygon>
                    (
                        blue, green,
                        std::back_inserter(output)
                    )
        );


    int i = 0;
    std::cout << "(blue \ green) u (green \ blue):" << std::endl;
    BOOST_FOREACH(polygon const& p, output)
    {
        std::cout << i++ << ": " << boost::geometry::area(p) << std::endl;
    }

    /*<-*/ create_svg("difference_inserter.svg", green, blue, output); /*->*/
    return 0;
}

//]


//[difference_inserter_output
/*`
Output:
[pre
(blue \\ green) u (green \\ blue):
0: 0.525154
1: 0.015
2: 0.181136
3: 0.128798
4: 0.340083
5: 0.307778
6: 0.02375
7: 0.542951
8: 0.0149697
9: 0.226855
10: 0.839424

[$img/algorithms/sym_difference.png]

]
*/
//]
