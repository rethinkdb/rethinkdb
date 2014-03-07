// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//#include <iostream>
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/concept/requires.hpp>

#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/included/test_exec_monitor.hpp>

#include <boost/geometry/geometries/geometries.hpp>

#include <boost/geometry/multi/algorithms/area.hpp>
#include <boost/geometry/multi/algorithms/length.hpp>
#include <boost/geometry/multi/algorithms/num_points.hpp>
#include <boost/geometry/multi/algorithms/perimeter.hpp>
#include <boost/geometry/multi/core/point_type.hpp>
#include <boost/geometry/multi/core/topological_dimension.hpp>
#include <boost/geometry/multi/geometries/multi_point.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#include <boost/geometry/multi/io/wkt/read.hpp>
#include <boost/geometry/multi/io/wkt/write.hpp>

template <typename T>
void test_all();


// Include the single test
#define GEOMETRY_TEST_MULTI
#include "io/wkt/wkt.cpp"

template <typename T>
void test_all()
{
    using namespace boost::geometry;
    typedef bg::model::point<T, 2, bg::cs::cartesian> P;

    test_wkt<bg::model::multi_point<P> >("multipoint((1 2),(3 4))", 2);
    test_wkt<bg::model::multi_linestring<bg::model::linestring<P> > >("multilinestring((1 1,2 2,3 3),(4 4,5 5,6 6))", 6, 4 * sqrt(2.0));
    test_wkt<bg::model::multi_polygon<bg::model::polygon<P> > >("multipolygon(((0 0,0 2,2 2,2 0,0 0),(1 1,1 2,2 2,2 1,1 1)),((0 0,0 4,4 4,4 0,0 0)))", 15, 0, 21, 28);

    // Support for the official alternative syntax for multipoint 
    // (provided by Aleksey Tulinov):
    test_relaxed_wkt<bg::model::multi_point<P> >("multipoint(1 2,3 4)", "multipoint((1 2),(3 4))");

    test_wrong_wkt<bg::model::multi_polygon<bg::model::polygon<P> > >(
        "MULTIPOLYGON(((0 0,0 2,2 2,2 0,0 0),(1 1,1 2,2 2,2 1,1 1)),(0 0,0 4,4 4,4 0,0 0)))",
        "expected '('");

    test_wrong_wkt<bg::model::multi_linestring<bg::model::linestring<P> > >(
        "MULTILINESTRING ((10 10, 20 20, 10 40), (40 40, 30 30, 40 20, 30 10)), (0 0, 1 1)", 
        "too much tokens at ','");

    test_wrong_wkt<bg::model::multi_point<P> >(
        "MULTIPOINT((8 9), 10 11)",
        "expected '(' at '10'");
    test_wrong_wkt<bg::model::multi_point<P> >(
        "MULTIPOINT(12 13, (14 15))",
        "bad lexical cast: source type value could not be interpreted as target at '(' in 'multipoint(12 13, (14 15))'");
    test_wrong_wkt<bg::model::multi_point<P> >(
        "MULTIPOINT((16 17), (18 19)",
        "expected ')' in 'multipoint((16 17), (18 19)'");
    test_wrong_wkt<bg::model::multi_point<P> >(
        "MULTIPOINT(16 17), (18 19)",
        "too much tokens at ',' in 'multipoint(16 17), (18 19)'");
}

/*

... see comments in "wkt.cpp"

union select 13,'# mpoint',npoints(geomfromtext('MULTIPOINT((1 2),(3 4))'))
union select 14,'length mpoint',length(geomfromtext('MULTIPOINT((1 2),(3 4))'))
union select 15,'peri mpoint',perimeter(geomfromtext('MULTIPOINT((1 2),(3 4))'))
union select 16,'area mpoint',area(geomfromtext('MULTIPOINT((1 2),(3 4))'))

union select 17,'# mls',npoints(geomfromtext('MULTILINESTRING((1 1,2 2,3 3),(4 4,5 5,6 6))'))
union select 18,'length mls',length(geomfromtext('MULTILINESTRING((1 1,2 2,3 3),(4 4,5 5,6 6))'))
union select 19,'peri mls',perimeter(geomfromtext('MULTILINESTRING((1 1,2 2,3 3),(4 4,5 5,6 6))'))
union select 20,'area mls',area(geomfromtext('MULTILINESTRING((1 1,2 2,3 3),(4 4,5 5,6 6))'))

union select 21,'# mpoly',npoints(geomfromtext('MULTIPOLYGON(((0 0,0 2,2 2,2 0,0 0),(1 1,1 2,2 2,2 1,1 1)),((0 0,0 4,4 4,4 0,0 0)))'))
union select 22,'length mpoly',length(geomfromtext('MULTIPOLYGON(((0 0,0 2,2 2,2 0,0 0),(1 1,1 2,2 2,2 1,1 1)),((0 0,0 4,4 4,4 0,0 0)))'))
union select 23,'peri mpoly',perimeter(geomfromtext('MULTIPOLYGON(((0 0,0 2,2 2,2 0,0 0),(1 1,1 2,2 2,2 1,1 1)),((0 0,0 4,4 4,4 0,0 0)))'))
union select 24,'area mpoly',area(geomfromtext('MULTIPOLYGON(((0 0,0 2,2 2,2 0,0 0),(1 1,1 2,2 2,2 1,1 1)),((0 0,0 4,4 4,4 0,0 0)))'))

*/
