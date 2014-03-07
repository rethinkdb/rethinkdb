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

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <boost/geometry/geometries/adapted/std_pair_as_segment.hpp>
#include <test_common/test_point.hpp>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)


template <typename P>
void test_2d()
{
    test_envelope<P>("POINT(1 1)", 1, 1, 1, 1);
    test_envelope<bg::model::linestring<P> >("LINESTRING(1 1,2 2)", 1, 2, 1, 2);
    test_envelope<bg::model::polygon<P> >("POLYGON((1 1,1 3,3 3,3 1,1 1))", 1, 3, 1, 3);

    test_envelope<bg::model::ring<P> >("POLYGON((1 1,1 3,3 3,3 1,1 1))", 1, 3, 1, 3);
    test_envelope<bg::model::box<P> >("BOX(1 1,3 3)", 1, 3, 1, 3);

    // Triangle, closed and open, and CCW.
    // Note that for the envelope algorithm,
    // these combinations should theoretically not differ
    test_envelope<bg::model::ring<P> >("POLYGON((4 1,0 7,7 9,4 1))", 0, 7, 1, 9);
    test_envelope<bg::model::ring<P, true, false> >("POLYGON((4 1,0 7,7 9))", 0, 7, 1, 9);
    test_envelope<bg::model::ring<P, false> >("POLYGON((4 1,7 9,0 7,4 1))", 0, 7, 1, 9);
    test_envelope<bg::model::ring<P, false, false> >("POLYGON((4 1,7 9,0 7))", 0, 7, 1, 9);

    typedef std::pair<P, P> segment_type;
    test_envelope<segment_type>("SEGMENT(1 1,3 3)", 1, 3, 1, 3);
}

template <typename P>
void test_3d()
{
    test_envelope<P>("POINT(1 2 3)", 1, 1, 2, 2, 3, 3);
    test_envelope<P>("POINT(3 2 1)", 3, 3, 2, 2, 1, 1);
    test_envelope<bg::model::linestring<P> >("LINESTRING(1 1 1,2 2 2)", 1, 2, 1, 2, 1, 2);
    test_envelope<bg::model::box<P> >("BOX(1 1 1,3 3 3)", 1, 3, 1, 3, 1, 3);
}


int test_main(int, char* [])
{
    //test_2d<int[2]>();
    //test_2d<float[2]>();
    //test_2d<double[2]>();
    test_2d<boost::tuple<float, float> >();
    test_2d<bg::model::d2::point_xy<int> >();
    test_2d<bg::model::d2::point_xy<float> >();
    test_2d<bg::model::d2::point_xy<double> >();

    test_3d<test::test_point>();
    test_3d<boost::tuple<int, int, int> >();

#ifdef HAVE_TTMATH
    test_2d<bg::model::d2::point_xy<ttmath_big> >();
    test_3d<boost::tuple<ttmath_big, ttmath_big, ttmath_big> >();
#endif

    return 0;
}
