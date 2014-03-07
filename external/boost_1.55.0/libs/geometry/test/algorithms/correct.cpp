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

#include <sstream>

#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/correct.hpp>
#include <boost/geometry/strategies/strategies.hpp>

#include <boost/geometry/io/dsv/write.hpp>

#include <boost/geometry/io/wkt/read.hpp>
#include <boost/geometry/io/wkt/write.hpp>

#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/ring.hpp>
#include <boost/geometry/geometries/polygon.hpp>


template <typename Geometry>
void test_geometry(std::string const& wkt, std::string const& expected)
{
    Geometry geometry;

    bg::read_wkt(wkt, geometry);
    bg::correct(geometry);

    std::ostringstream out;
    out << bg::wkt(geometry);

    BOOST_CHECK_EQUAL(out.str(), expected);
}


// Note: 3D/box test cannot be done using WKT because:
// -> wkt-box does not exist
// -> so it is converted to a ring
// -> ring representation of 3d-box is not supported, nor feasible
// -> so it uses DSV which can represent a box
template <typename Geometry>
void test_geometry_dsv(std::string const& wkt, std::string const& expected)
{
    Geometry geometry;

    bg::read_wkt(wkt, geometry);
    bg::correct(geometry);

    std::ostringstream out;
    out << bg::dsv(geometry);

    BOOST_CHECK_EQUAL(out.str(), expected);
}






template <typename P>
void test_all()
{
    // Define clockwise and counter clockwise polygon
    std::string cw_ring       = "POLYGON((0 0,0 1,1 1,1 0,0 0))";
    std::string ccw_ring      = "POLYGON((0 0,1 0,1 1,0 1,0 0))";
    std::string cw_open_ring  = "POLYGON((0 0,0 1,1 1,1 0))";
    std::string ccw_open_ring = "POLYGON((0 0,1 0,1 1,0 1))";

    // already cw_ring
    test_geometry<bg::model::ring<P> >(cw_ring, cw_ring);

    // wrong order
    test_geometry<bg::model::ring<P> >(ccw_ring, cw_ring);

    // ccw-ring, input ccw-ring, already correct
    test_geometry<bg::model::ring<P, false> >(ccw_ring, ccw_ring);

    // ccw-ring, input cw-ring, corrected
    test_geometry<bg::model::ring<P, false> >(cw_ring, ccw_ring);

    // open-ring, input ccw-ring, already correct
    test_geometry<bg::model::ring<P, true, false> >(cw_open_ring, cw_open_ring);

    // ccw-ring, input cw-ring, corrected
    test_geometry<bg::model::ring<P, true, false> >(ccw_open_ring, "POLYGON((0 1,1 1,1 0,0 0))");



    // not closed
    test_geometry<bg::model::ring<P> >(
            ccw_open_ring,
            cw_ring);

    // counter clockwise, cw_ring
    test_geometry<bg::model::ring<P, false> >(ccw_ring, ccw_ring);

    test_geometry<bg::model::ring<P, false> >(cw_ring, ccw_ring);


    // polygon: cw_ring
    test_geometry<bg::model::polygon<P> >(cw_ring, cw_ring);
    // wrong order
    test_geometry<bg::model::polygon<P> >(
            "POLYGON((0 0,1 0,1 1,0 1,0 0))",
            cw_ring);
    // wrong order & not closed
    test_geometry<bg::model::polygon<P> >(
            ccw_open_ring,
            cw_ring);


    std::string cw_holey_polygon =
            "POLYGON((0 0,0 4,4 4,4 0,0 0),(1 1,2 1,2 2,1 2,1 1))";
    std::string ccw_holey_polygon =
            "POLYGON((0 0,4 0,4 4,0 4,0 0),(1 1,1 2,2 2,2 1,1 1))";

    // with holes: cw_ring
    test_geometry<bg::model::polygon<P> >(
            cw_holey_polygon,
            cw_holey_polygon);
    // wrong order of main
    test_geometry<bg::model::polygon<P> >(
            "POLYGON((0 0,4 0,4 4,0 4,0 0),(1 1,2 1,2 2,1 2,1 1))",
            cw_holey_polygon);
    // wrong order of hole
    test_geometry<bg::model::polygon<P> >(
            "POLYGON((0 0,0 4,4 4,4 0,0 0),(1 1,1 2,2 2,2 1,1 1))",
            cw_holey_polygon);

    // wrong order of main and hole
    test_geometry<bg::model::polygon<P> >(ccw_holey_polygon, cw_holey_polygon);

    // test the counter-clockwise
    test_geometry<bg::model::polygon<P, false> >(
            ccw_holey_polygon, ccw_holey_polygon);

    // Boxes
    std::string proper_box = "POLYGON((0 0,0 2,2 2,2 0,0 0))";
    test_geometry<bg::model::box<P> >(proper_box, proper_box);
    test_geometry<bg::model::box<P> >("BOX(0 0,2 2)", proper_box);
    test_geometry<bg::model::box<P> >("BOX(2 2,0 0)", proper_box);
    test_geometry<bg::model::box<P> >("BOX(0 2,2 0)", proper_box);

    // Cubes
    typedef bg::model::box<bg::model::point<double, 3, bg::cs::cartesian> > box3d;
    std::string proper_3d_dsv_box = "((0, 0, 0), (2, 2, 2))";
    test_geometry_dsv<box3d>("BOX(0 0 0,2 2 2)", proper_3d_dsv_box);
    test_geometry_dsv<box3d>("BOX(2 2 2,0 0 0)", proper_3d_dsv_box);
    test_geometry_dsv<box3d>("BOX(0 2 2,2 0 0)", proper_3d_dsv_box);
    test_geometry_dsv<box3d>("BOX(2 0 2,0 2 0)", proper_3d_dsv_box);
    test_geometry_dsv<box3d>("BOX(0 0 2,2 2 0)", proper_3d_dsv_box);
}


int test_main(int, char* [])
{
    //test_all<int[2]>();
    //test_all<float[2]>(); not yet because cannot be copied, for polygon
    //test_all<double[2]>();

    test_all<bg::model::d2::point_xy<int> >();
    test_all<bg::model::d2::point_xy<float> >();
    test_all<bg::model::d2::point_xy<double> >();

#if defined(HAVE_TTMATH)
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    return 0;
}
