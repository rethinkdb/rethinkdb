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


#include <algorithms/test_area.hpp>

#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/ring.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include <test_geometries/all_custom_ring.hpp>
#include <test_geometries/all_custom_polygon.hpp>
//#define GEOMETRY_TEST_DEBUG

#include <boost/variant/variant.hpp>

template <typename Polygon>
void test_polygon()
{
    // Rotated square, length=sqrt(2) -> area=2
    test_geometry<Polygon>("POLYGON((1 1,2 2,3 1,2 0,1 1))", 2.0);
    test_geometry<Polygon>("POLYGON((1 1,2 2,3 1,2 0,1 1))", 2.0);
    test_geometry<Polygon>("POLYGON((0 0,0 7,4 2,2 0,0 0))", 16.0);
    test_geometry<Polygon>("POLYGON((1 1,2 1,2 2,1 2,1 1))", -1.0);
    test_geometry<Polygon>("POLYGON((0 0,0 7,4 2,2 0,0 0), (1 1,2 1,2 2,1 2,1 1))", 15.0);
}


template <typename P>
void test_all()
{
    test_geometry<bg::model::box<P> >("POLYGON((0 0,2 2))", 4.0);
    test_geometry<bg::model::box<P> >("POLYGON((2 2,0 0))", 4.0);

    test_polygon<bg::model::polygon<P> >();
    test_polygon<all_custom_polygon<P> >();

    // clockwise rings (second is wrongly ordered)
    test_geometry<bg::model::ring<P> >("POLYGON((0 0,0 7,4 2,2 0,0 0))", 16.0);
    test_geometry<bg::model::ring<P> >("POLYGON((0 0,2 0,4 2,0 7,0 0))", -16.0);

    test_geometry<all_custom_ring<P> >("POLYGON((0 0,0 7,4 2,2 0,0 0))", 16.0);

    // ccw
    test_geometry<bg::model::polygon<P, false> >
            ("POLYGON((0 0,0 7,4 2,2 0,0 0), (1 1,2 1,2 2,1 2,1 1))", -15.0);
}

template <typename Point>
void test_spherical(bool polar = false)
{
    typedef typename bg::coordinate_type<Point>::type ct;
    bg::model::polygon<Point> geometry;

    // unit-sphere has area of 4-PI. Polygon covering 1/8 of it:
    // calculations splitted for ttmath
    ct const four = 4.0;
    ct const eight = 8.0;
    ct expected = four * boost::geometry::math::pi<ct>() / eight;
    bg::read_wkt("POLYGON((0 0,0 90,90 0,0 0))", geometry);

    ct area = bg::area(geometry);
    BOOST_CHECK_CLOSE(area, expected, 0.0001);

    // With strategy, radius 2 -> 4 pi r^2
    bg::strategy::area::huiller
        <
            typename bg::point_type<Point>::type
        > strategy(2.0);

    area = bg::area(geometry, strategy);
    ct const two = 2.0;
    BOOST_CHECK_CLOSE(area, two * two * expected, 0.0001);

    // Wrangel Island (dateline crossing)
    // With (spherical) Earth strategy
    bg::strategy::area::huiller
        <
            typename bg::point_type<Point>::type
        > spherical_earth(6373);
    bg::read_wkt("POLYGON((-178.7858 70.7852, 177.4758 71.2333, 179.7436 71.5733, -178.7858 70.7852))", geometry);
    area = bg::area(geometry, spherical_earth);
    // SQL Server gives: 4537.9654419375
    // PostGIS gives: 4537.9311668307
    // Note: those are Geographic, this test is Spherical
    BOOST_CHECK_CLOSE(area, 4506.6389, 0.001); 

    // Wrangel, more in detail
    bg::read_wkt("POLYGON((-178.568604 71.564148,-178.017548 71.449692,-177.833313 71.3461,-177.502838 71.277466 ,-177.439453 71.226929,-177.620026 71.116638,-177.9389 71.037491,-178.8186 70.979965,-179.274445 70.907761,-180 70.9972,179.678314 70.895538,179.272766 70.888596,178.791016 70.7964,178.617737 71.035538,178.872192 71.217484,179.530273 71.4383 ,-180 71.535843 ,-179.628601 71.577194,-179.305298 71.551361,-179.03421 71.597748,-178.568604 71.564148))", geometry);
    area = bg::area(geometry, spherical_earth);
    // SQL Server gives: 7669.10402181435
    // PostGIS gives: 7669.55565459832
    BOOST_CHECK_CLOSE(area, 7616.523769, 0.001); 

    // Check more at the equator
    /*
    select 1,geography::STGeomFromText('POLYGON((-178.7858 10.7852 , 179.7436 11.5733 , 177.4758 11.2333 , -178.7858 10.7852))',4326) .STArea()/1000000.0
    union select 2,geography::STGeomFromText('POLYGON((-178.7858 20.7852 , 179.7436 21.5733 , 177.4758 21.2333 , -178.7858 20.7852))',4326) .STArea()/1000000.0
    union select 3,geography::STGeomFromText('POLYGON((-178.7858 30.7852 , 179.7436 31.5733 , 177.4758 31.2333 , -178.7858 30.7852))',4326) .STArea()/1000000.0
    union select 0,geography::STGeomFromText('POLYGON((-178.7858 0.7852 , 179.7436 1.5733 , 177.4758 1.2333 , -178.7858 0.7852))',4326) .STArea()/1000000.0
    union select 4,geography::STGeomFromText('POLYGON((-178.7858 40.7852 , 179.7436 41.5733 , 177.4758 41.2333 , -178.7858 40.7852))',4326) .STArea()/1000000.0
    */

    bg::read_wkt("POLYGON((-178.7858 0.7852, 177.4758 1.2333, 179.7436 1.5733, -178.7858 0.7852))", geometry);
    area = bg::area(geometry, spherical_earth);
    BOOST_CHECK_CLOSE(area, 14136.09946, 0.001); // SQL Server gives: 14064.1902284513


    bg::read_wkt("POLYGON((-178.7858 10.7852, 177.4758 11.2333, 179.7436 11.5733, -178.7858 10.7852))", geometry);
    area = bg::area(geometry, spherical_earth);
    BOOST_CHECK_CLOSE(area, 13760.2456, 0.001); // SQL Server gives: 13697.0941155193

    bg::read_wkt("POLYGON((-178.7858 20.7852, 177.4758 21.2333, 179.7436 21.5733, -178.7858 20.7852))", geometry);
    area = bg::area(geometry, spherical_earth);
    BOOST_CHECK_CLOSE(area, 12987.8682, 0.001); // SQL Server gives: 12944.3970990317 -> -39m^2 

    bg::read_wkt("POLYGON((-178.7858 30.7852, 177.4758 31.2333, 179.7436 31.5733, -178.7858 30.7852))", geometry);
    area = bg::area(geometry, spherical_earth);
    BOOST_CHECK_CLOSE(area, 11856.3935, 0.001); // SQL Server gives: 11838.5338423574 -> -18m^2

    bg::read_wkt("POLYGON((-178.7858 40.7852, 177.4758 41.2333, 179.7436 41.5733, -178.7858 40.7852))", geometry);
    area = bg::area(geometry, spherical_earth);
    BOOST_CHECK_CLOSE(area, 10404.627685523914, 0.001); // SQL Server gives: 10412.0607137119, -> +8m^2

    // Concave
    bg::read_wkt("POLYGON((0 40,1 42,0 44,2 43,4 44,3 42,4 40,2 41,0 40))", geometry);
    area = bg::area(geometry, spherical_earth);
    BOOST_CHECK_CLOSE(area, 73538.2958, 0.001); // SQL Server gives: 73604.2047689719

    // With hole POLYGON((0 40,4 40,4 44,0 44,0 40),(1 41,2 43,3 42,1 41))
    bg::read_wkt("POLYGON((0 40,0 44,4 44,4 40,0 40),(1 41,3 42,2 43,1 41))", geometry);
    area = bg::area(geometry, spherical_earth);
    BOOST_CHECK_CLOSE(area, 133233.844876, 0.001); // SQL Server gives: 133353.335

    {
        bg::model::ring<Point> aurha; // a'dam-utr-rott.-den haag-a'dam
        bg::read_wkt("POLYGON((4.892 52.373,5.119 52.093,4.479 51.930,4.23 52.08,4.892 52.373))", aurha);
        if (polar)
        {
            // Create colatitudes (measured from pole)
            BOOST_FOREACH(Point& p, aurha)
            {
                bg::set<1>(p, ct(90) - bg::get<1>(p));
            }
            bg::correct(aurha);
        }
        bg::strategy::area::huiller
            <
                typename bg::point_type<Point>::type
            > huiller(6372.795);
        area = bg::area(aurha, huiller);
        BOOST_CHECK_CLOSE(area, 1476.645675, 0.0001);

        // SQL Server gives: 1481.55595960659
        // for select geography::STGeomFromText('POLYGON((4.892 52.373,4.23 52.08,4.479 51.930,5.119 52.093,4.892 52.373))',4326).STArea()/1000000.0
    }
}

template <typename P>
void test_ccw()
{
    typedef bg::model::polygon<P, false> ccw_polygon;
    // counterclockwise rings (second is wrongly ordered)
    test_geometry<ccw_polygon>("POLYGON((1 1,2 2,3 1,2 0,1 1))", -2.0);
    test_geometry<ccw_polygon>("POLYGON((1 1,2 0,3 1,2 2,1 1))", +2.0);
    test_geometry<ccw_polygon>("POLYGON((0 0,0 7,4 2,2 0,0 0))", -16.0);
    test_geometry<ccw_polygon>("POLYGON((0 0,2 0,4 2,0 7,0 0))", +16.0);
}

template <typename P>
void test_open()
{
    typedef bg::model::polygon<P, true, false> open_polygon;
    test_geometry<open_polygon>("POLYGON((1 1,2 2,3 1,2 0))", 2.0);
    // Note the triangular testcase used in CCW is not sensible for open/close
}

template <typename P>
void test_open_ccw()
{
    typedef bg::model::polygon<P, false, false> open_polygon;
    test_geometry<open_polygon>("POLYGON((1 1,2 0,3 1,2 2))", 2.0);
    // Note the triangular testcase used in CCW is not sensible for open/close
}

template <typename P>
void test_empty_input()
{
    bg::model::polygon<P> poly_empty;
    bg::model::ring<P> ring_empty;

    test_empty_input(poly_empty);
    test_empty_input(ring_empty);
}

void test_large_integers()
{
    typedef bg::model::point<int, 2, bg::cs::cartesian> int_point_type;
    typedef bg::model::point<double, 2, bg::cs::cartesian> double_point_type;

    bg::model::polygon<int_point_type> int_poly;
    bg::model::polygon<double_point_type> double_poly;

    std::string const polygon_li = "POLYGON((1872000 528000,1872000 192000,1536119 192000,1536000 528000,1200000 528000,1200000 863880,1536000 863880,1872000 863880,1872000 528000))";
    bg::read_wkt(polygon_li, int_poly);
    bg::read_wkt(polygon_li, double_poly);

    double int_area = bg::area(int_poly);
    double double_area = bg::area(double_poly);

    BOOST_CHECK_CLOSE(int_area, double_area, 0.0001);
}

void test_variant()
{
    typedef bg::model::point<double, 2, bg::cs::cartesian> double_point_type;
    typedef bg::model::polygon<double_point_type> polygon_type;
    typedef bg::model::box<double_point_type> box_type;

    polygon_type poly;
    std::string const polygon_li = "POLYGON((18 5,18 1,15 1,15 5,12 5,12 8,15 8,18 8,18 5))";
    bg::read_wkt(polygon_li, poly);

    box_type box;
    std::string const box_li = "BOX(0 0,2 2)";
    bg::read_wkt(box_li, box);

    boost::variant<polygon_type, box_type> v;

    v = poly;
    BOOST_CHECK_CLOSE(bg::area(v), bg::area(poly), 0.0001);
    v = box;
    BOOST_CHECK_CLOSE(bg::area(v), bg::area(box), 0.0001);
}

int test_main(int, char* [])
{
    test_all<bg::model::point<int, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<float, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<double, 2, bg::cs::cartesian> >();

    test_spherical<bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree> > >();
    //test_spherical<bg::model::point<double, 2, bg::cs::spherical<bg::degree> > >(true);

    test_ccw<bg::model::point<double, 2, bg::cs::cartesian> >();
    test_open<bg::model::point<double, 2, bg::cs::cartesian> >();
    test_open_ccw<bg::model::point<double, 2, bg::cs::cartesian> >();

#ifdef HAVE_TTMATH
    test_all<bg::model::d2::point_xy<ttmath_big> >();
    test_spherical<bg::model::point<ttmath_big, 2, bg::cs::spherical_equatorial<bg::degree> > >();
#endif

    test_large_integers();

    test_variant();

    // test_empty_input<bg::model::d2::point_xy<int> >();

    return 0;
}
