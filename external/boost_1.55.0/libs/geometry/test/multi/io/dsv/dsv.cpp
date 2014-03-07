// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <sstream>

#include <geometry_test_common.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/multi/io/dsv/write.hpp>
#include <boost/geometry/multi/geometries/multi_geometries.hpp>
#include <boost/geometry/multi/io/wkt/read.hpp>

template <typename Geometry>
void test_dsv(std::string const& wkt, std::string const& expected, bool json = false)
{
    Geometry geometry;
    bg::read_wkt(wkt, geometry);
    std::ostringstream out;
    if (json)
    {
        out << bg::dsv(geometry, ", ", "[", "]", ", ", "[ ", " ]", ", ");
    }
    else
    {
        out << bg::dsv(geometry);
    }
    BOOST_CHECK_EQUAL(out.str(), expected);
}


template <typename T>
void test_all()
{
    using namespace boost::geometry;
    typedef model::point<T, 2, cs::cartesian> point_type;
    typedef model::multi_point<point_type> mpoint;
    typedef model::multi_linestring<model::linestring<point_type> > mline;
    typedef model::multi_polygon<model::polygon<point_type> > mpoly;

    test_dsv<mpoint>
        (
            "multipoint((1 2),(3 4))", 
            "((1, 2), (3, 4))"
        );
    test_dsv<mline>
        (
            "multilinestring((1 1,2 2,3 3),(4 4,5 5,6 6))",
            "(((1, 1), (2, 2), (3, 3)), ((4, 4), (5, 5), (6, 6)))"
        );
    test_dsv<mpoly>
        (
            // Multi with 2 poly's, first has hole, second is triangle
            "multipolygon(((0 0,0 4,4 4,4 0,0 0),(1 1,1 2,2 2,2 1,1 1)),((5 5,6 5,5 6,5 5)))",
            "((((0, 0), (0, 4), (4, 4), (4, 0), (0, 0)), ((1, 1), (1, 2), (2, 2), (2, 1), (1, 1))), (((5, 5), (6, 5), (5, 6), (5, 5))))"
        );

    // http://geojson.org/geojson-spec.html#id5
    test_dsv<mpoint>
        (
            "multipoint((1 2),(3 4))", 
            "[ [1, 2], [3, 4] ]",
            true
        );

    // http://geojson.org/geojson-spec.html#id6
    test_dsv<mline>
        (
            "multilinestring((1 1,2 2,3 3),(4 4,5 5,6 6))",
            "[ [ [1, 1], [2, 2], [3, 3] ], [ [4, 4], [5, 5], [6, 6] ] ]",
            true
        );

    // http://geojson.org/geojson-spec.html#id7
    test_dsv<mpoly>
        (
            "multipolygon(((0 0,0 4,4 4,4 0,0 0),(1 1,1 2,2 2,2 1,1 1)),((5 5,6 5,5 6,5 5)))",
            "[ [ [ [0, 0], [0, 4], [4, 4], [4, 0], [0, 0] ], [ [1, 1], [1, 2], [2, 2], [2, 1], [1, 1] ] ], [ [ [5, 5], [6, 5], [5, 6], [5, 5] ] ] ]",
            true
        );

}


int test_main(int, char* [])
{
    test_all<double>();
    test_all<int>();

    return 0;
}

