// Boost.Geometry Index
//
// Quickbook Examples
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[rtree_polygons_shared_ptr

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include <boost/geometry/index/rtree.hpp>

#include <cmath>
#include <vector>
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

int main(void)
{
    typedef bg::model::point<float, 2, bg::cs::cartesian> point;
    typedef bg::model::box<point> box;
    typedef bg::model::polygon<point, false, false> polygon; // ccw, open polygon
    typedef boost::shared_ptr<polygon> shp;
    typedef std::pair<box, shp> value;

    // create the rtree using default constructor
    bgi::rtree< value, bgi::linear<16, 4> > rtree;

    std::cout << "filling index with polygons shared pointers:" << std::endl;

    // create some polygons and fill the spatial index
    for ( unsigned i = 0 ; i < 10 ; ++i )
    {
        // create a polygon
        shp p(new polygon());
        for ( float a = 0 ; a < 6.28316f ; a += 1.04720f )
        {
            float x = i + int(10*::cos(a))*0.1f;
            float y = i + int(10*::sin(a))*0.1f;
            p->outer().push_back(point(x, y));
        }

        // display new polygon
        std::cout << bg::wkt<polygon>(*p) << std::endl;

        // calculate polygon bounding box
        box b = bg::return_envelope<box>(*p);
        // insert new value
        rtree.insert(std::make_pair(b, p));
    }

    // find values intersecting some area defined by a box
    box query_box(point(0, 0), point(5, 5));
    std::vector<value> result_s;
    rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));

    // find 5 nearest values to a point
    std::vector<value> result_n;
    rtree.query(bgi::nearest(point(0, 0), 5), std::back_inserter(result_n));

    // display results
    std::cout << "spatial query box:" << std::endl;
    std::cout << bg::wkt<box>(query_box) << std::endl;
    std::cout << "spatial query result:" << std::endl;
    BOOST_FOREACH(value const& v, result_s)
        std::cout << bg::wkt<polygon>(*v.second) << std::endl;

    std::cout << "knn query point:" << std::endl;
    std::cout << bg::wkt<point>(point(0, 0)) << std::endl;
    std::cout << "knn query result:" << std::endl;
    BOOST_FOREACH(value const& v, result_n)
        std::cout << bg::wkt<polygon>(*v.second) << std::endl;

    return 0;
}

//]
