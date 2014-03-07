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


#include <algorithms/test_envelope.hpp>


#include <boost/geometry/multi/algorithms/envelope.hpp>
#include <boost/geometry/multi/geometries/multi_point.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#include <boost/geometry/multi/io/wkt/read.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <test_common/test_point.hpp>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)


template <typename P>
void test_2d()
{
    /*test_envelope<bg::model::multi_point<P> >(
            "MULTIPOINT((1 1),(1 0),(1 2))", 1, 1, 0, 2);
    test_envelope<bg::model::multi_linestring<bg::model::linestring<P> > >(
            "MULTILINESTRING((0 0,1 1),(1 1,2 2),(2 2,3 3))", 0, 3, 0, 3);
*/
    test_envelope<bg::model::multi_polygon<bg::model::polygon<P> > >(
            "MULTIPOLYGON(((1 1,1 3,3 3,3 1,1 1)),((4 4,4 6,6 6,6 4,4 4)))", 1, 6, 1, 6);
}


template <typename P>
void test_3d()
{
    typedef bg::model::multi_point<P> mp;
}


int test_main( int , char* [] )
{
    test_2d<boost::tuple<float, float> >();
    test_2d<bg::model::d2::point_xy<float> >();
    test_2d<bg::model::d2::point_xy<double> >();

    test_3d<boost::tuple<float, float, float> >();
    test_3d<bg::model::point<double, 3, bg::cs::cartesian> >();

#ifdef HAVE_TTMATH
    test_2d<bg::model::d2::point_xy<ttmath_big> >();
    test_2d<bg::model::point<ttmath_big, 3, bg::cs::cartesian> >();
#endif

    return 0;
}
