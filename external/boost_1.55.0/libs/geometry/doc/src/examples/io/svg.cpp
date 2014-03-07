// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2013 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[svg_mapper
//` Shows the usage of svg_mapper

#include <iostream>
#include <fstream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

int main()
{
    // Specify the basic type
    typedef boost::geometry::model::d2::point_xy<double> point_type;

    // Declare some geometries and set their values
    point_type a;
    boost::geometry::assign_values(a, 3, 6);

    boost::geometry::model::polygon<point_type> b;
    boost::geometry::read_wkt("POLYGON((0 0,0 7,4 2,2 0,0 0))", b);

    boost::geometry::model::linestring<point_type> c;
    c.push_back(point_type(3, 4));
    c.push_back(point_type(4, 5));

    // Declare a stream and an SVG mapper
    std::ofstream svg("my_map.svg");
    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    
    // Add geometries such that all these geometries fit on the map
    mapper.add(a);
    mapper.add(b);
    mapper.add(c);

    // Draw the geometries on the SVG map, using a specific SVG style
    mapper.map(a, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:2", 5);
    mapper.map(b, "fill-opacity:0.3;fill:rgb(51,51,153);stroke:rgb(51,51,153);stroke-width:2");
    mapper.map(c, "opacity:0.4;fill:none;stroke:rgb(212,0,0);stroke-width:5");
    
    // Destructor of map will be called - adding </svg>
    // Destructor of stream will be called, closing the file

    return 0;
}

//]


//[svg_mapper_output
/*`
Output:

[$img/io/svg_mapper.png]
*/
//]
