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


#include <geometry_test_common.hpp>

#include <boost/geometry/strategies/strategy_transform.hpp>
#include <boost/geometry/algorithms/transform.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

template <typename T, typename P>
inline T check_distance(P const& p)
{
    T x = bg::get<0>(p);
    T y = bg::get<1>(p);
    T z = bg::get<2>(p);
    return sqrt(x * x + y * y + z * z);
}

template <typename T>
void test_transformations_spherical()
{
    T const input_long = 15.0;
    T const input_lat = 5.0;

    T const expected_long = 0.26179938779914943653855361527329;
    T const expected_lat = 0.08726646259971647884618453842443;

    // Can be checked using http://www.calc3d.com/ejavascriptcoordcalc.html
    // (for phi use long, in radians, for theta use lat, in radians, they are listed there as "theta, phi")
    T const expected_polar_x = 0.084186;
    T const expected_polar_y = 0.0225576;
    T const expected_polar_z = 0.996195;

    // Can be checked with same URL using 90-theta for lat.
    // So for theta use 85 degrees, in radians: 0.08726646259971647884618453842443
    T const expected_equatorial_x = 0.962250;
    T const expected_equatorial_y = 0.257834;
    T const expected_equatorial_z = 0.0871557;

    // 1: Spherical-polar (lat=5, so it is near the pole - on a unit sphere)
    bg::model::point<T, 2, bg::cs::spherical<bg::degree> > sp(input_long, input_lat);

    // 1a: to radian
    bg::model::point<T, 2, bg::cs::spherical<bg::radian> > spr;
    bg::transform(sp, spr);
    BOOST_CHECK_CLOSE(bg::get<0>(spr), expected_long, 0.001);
    BOOST_CHECK_CLOSE(bg::get<1>(spr), expected_lat, 0.001);

    // 1b: to cartesian-3d
    bg::model::point<T, 3, bg::cs::cartesian> pc3;
    bg::transform(sp, pc3);
    BOOST_CHECK_CLOSE(bg::get<0>(pc3), expected_polar_x, 0.001);
    BOOST_CHECK_CLOSE(bg::get<1>(pc3), expected_polar_y, 0.001);
    BOOST_CHECK_CLOSE(bg::get<2>(pc3), expected_polar_z, 0.001);
    BOOST_CHECK_CLOSE(check_distance<T>(pc3), 1.0, 0.001);

    // 1c: back
    bg::transform(pc3, spr);
    BOOST_CHECK_CLOSE(bg::get<0>(spr), expected_long, 0.001);
    BOOST_CHECK_CLOSE(bg::get<1>(spr), expected_lat, 0.001);

    // 2: Spherical-equatorial (lat=5, so it is near the equator)
    bg::model::point<T, 2, bg::cs::spherical_equatorial<bg::degree> > se(input_long, input_lat);

    // 2a: to radian 
    bg::model::point<T, 2, bg::cs::spherical_equatorial<bg::radian> > ser;
    bg::transform(se, ser);
    BOOST_CHECK_CLOSE(bg::get<0>(ser), expected_long, 0.001);
    BOOST_CHECK_CLOSE(bg::get<1>(ser), expected_lat, 0.001);

    bg::transform(se, pc3);
    BOOST_CHECK_CLOSE(bg::get<0>(pc3), expected_equatorial_x, 0.001);
    BOOST_CHECK_CLOSE(bg::get<1>(pc3), expected_equatorial_y, 0.001);
    BOOST_CHECK_CLOSE(bg::get<2>(pc3), expected_equatorial_z, 0.001);
    BOOST_CHECK_CLOSE(check_distance<T>(pc3), 1.0, 0.001);

    // 2c: back
    bg::transform(pc3, ser);
    BOOST_CHECK_CLOSE(bg::get<0>(spr), expected_long, 0.001);  // expected_long
    BOOST_CHECK_CLOSE(bg::get<1>(spr), expected_lat, 0.001); // expected_lat


    // 3: Spherical-polar including radius
    bg::model::point<T, 3, bg::cs::spherical<bg::degree> > sp3(input_long, input_lat, 0.5);

    // 3a: to radian
    bg::model::point<T, 3, bg::cs::spherical<bg::radian> > spr3;
    bg::transform(sp3, spr3);
    BOOST_CHECK_CLOSE(bg::get<0>(spr3), expected_long, 0.001);
    BOOST_CHECK_CLOSE(bg::get<1>(spr3), expected_lat, 0.001);
    BOOST_CHECK_CLOSE(bg::get<2>(spr3), 0.5, 0.001);

    // 3b: to cartesian-3d
    bg::transform(sp3, pc3);
    BOOST_CHECK_CLOSE(bg::get<0>(pc3), expected_polar_x / 2.0, 0.001);
    BOOST_CHECK_CLOSE(bg::get<1>(pc3), expected_polar_y / 2.0, 0.001);
    BOOST_CHECK_CLOSE(bg::get<2>(pc3), expected_polar_z / 2.0, 0.001);
    BOOST_CHECK_CLOSE(check_distance<T>(pc3), 0.5, 0.001);

    // 3c: back
    bg::transform(pc3, spr3);
    BOOST_CHECK_CLOSE(bg::get<0>(spr3), expected_long, 0.001);
    BOOST_CHECK_CLOSE(bg::get<1>(spr3), expected_lat, 0.001);
    BOOST_CHECK_CLOSE(bg::get<2>(spr3), 0.5, 0.001);


    // 4: Spherical-equatorial including radius
    bg::model::point<T, 3, bg::cs::spherical_equatorial<bg::degree> > se3(input_long, input_lat, 0.5);

    // 4a: to radian
    bg::model::point<T, 3, bg::cs::spherical_equatorial<bg::radian> > ser3;
    bg::transform(se3, ser3);
    BOOST_CHECK_CLOSE(bg::get<0>(ser3), expected_long, 0.001);
    BOOST_CHECK_CLOSE(bg::get<1>(ser3), expected_lat, 0.001);
    BOOST_CHECK_CLOSE(bg::get<2>(ser3), 0.5, 0.001);

    // 4b: to cartesian-3d
    bg::transform(se3, pc3);
    BOOST_CHECK_CLOSE(bg::get<0>(pc3), expected_equatorial_x / 2.0, 0.001);
    BOOST_CHECK_CLOSE(bg::get<1>(pc3), expected_equatorial_y / 2.0, 0.001);
    BOOST_CHECK_CLOSE(bg::get<2>(pc3), expected_equatorial_z / 2.0, 0.001);
    BOOST_CHECK_CLOSE(check_distance<T>(pc3), 0.5, 0.001);

    // 4c: back
    bg::transform(pc3, ser3);
    BOOST_CHECK_CLOSE(bg::get<0>(ser3), expected_long, 0.001);
    BOOST_CHECK_CLOSE(bg::get<1>(ser3), expected_lat, 0.001);
    BOOST_CHECK_CLOSE(bg::get<2>(ser3), 0.5, 0.001);
}

int test_main(int, char* [])
{
    test_transformations_spherical<double>();

    return 0;
}
