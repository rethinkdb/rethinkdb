// Boost.Geometry Index
//
// Quickbook Examples
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[rtree_variants_map

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/ring.hpp>
#include <boost/geometry/geometries/linestring.hpp>

#include <boost/geometry/index/rtree.hpp>

#include <cmath>
#include <vector>
#include <map>
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/variant.hpp>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::point<float, 2, bg::cs::cartesian> point;
typedef bg::model::box<point> box;
typedef bg::model::polygon<point, false, false> polygon; // ccw, open polygon
typedef bg::model::ring<point, false, false> ring; // ccw, open ring
typedef bg::model::linestring<point> linestring;
typedef boost::variant<polygon, ring, linestring> geometry;

typedef std::map<unsigned, geometry> map;
typedef std::pair<box, map::iterator> value;

template <class Container>
void fill(unsigned i, Container & container)
{
    for ( float a = 0 ; a < 6.28316f ; a += 1.04720f )
    {
        float x = i + int(10*::cos(a))*0.1f;
        float y = i + int(10*::sin(a))*0.1f;
        container.push_back(point(x, y));
    }
}

struct print_visitor : public boost::static_visitor<>
{
    void operator()(polygon const& g) const { std::cout << bg::wkt<polygon>(g) << std::endl; }
    void operator()(ring const& g) const { std::cout << bg::wkt<ring>(g) << std::endl; }
    void operator()(linestring const& g) const { std::cout << bg::wkt<linestring>(g) << std::endl; }
};

struct envelope_visitor : public boost::static_visitor<box>
{
    box operator()(polygon const& g) const { return bg::return_envelope<box>(g); }
    box operator()(ring const& g) const { return bg::return_envelope<box>(g); }
    box operator()(linestring const& g) const { return bg::return_envelope<box>(g); }
};


int main(void)
{
    // geometries container
    map geometries;

    // create some geometries
    for ( unsigned i = 0 ; i < 10 ; ++i )
    {
        unsigned c = rand() % 3;

        if ( 0 == c )
        {
            // create polygon
            polygon p;
            fill(i, p.outer());
            geometries.insert(std::make_pair(i, geometry(p)));
        }
        else if ( 1 == c )
        {
            // create ring
            ring r;
            fill(i, r);
            geometries.insert(std::make_pair(i, geometry(r)));
        }
        else if ( 2 == c )
        {
            // create linestring
            linestring l;
            fill(i, l);
            geometries.insert(std::make_pair(i, geometry(l)));
        }
    }

    // display geometries
    std::cout << "generated geometries:" << std::endl;
    BOOST_FOREACH(map::value_type const& p, geometries)
        boost::apply_visitor(print_visitor(), p.second);

    // create the rtree using default constructor
    bgi::rtree< value, bgi::quadratic<16, 4> > rtree;

    // fill the spatial index
    for ( map::iterator it = geometries.begin() ; it != geometries.end() ; ++it )
    {
        // calculate polygon bounding box
        box b = boost::apply_visitor(envelope_visitor(), it->second);
        // insert new value
        rtree.insert(std::make_pair(b, it));
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
        boost::apply_visitor(print_visitor(), v.second->second);

    std::cout << "knn query point:" << std::endl;
    std::cout << bg::wkt<point>(point(0, 0)) << std::endl;
    std::cout << "knn query result:" << std::endl;
    BOOST_FOREACH(value const& v, result_n)
        boost::apply_visitor(print_visitor(), v.second->second);

    return 0;
}

//]
