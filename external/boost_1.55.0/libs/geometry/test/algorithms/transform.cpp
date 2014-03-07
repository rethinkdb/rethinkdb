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

#include <iostream>
#include <sstream>


#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/make.hpp>
#include <boost/geometry/algorithms/transform.hpp>
#include <boost/geometry/strategies/strategies.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

#include <boost/geometry/io/dsv/write.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>

#include <test_common/test_point.hpp>

template <typename P1, typename P2>
void test_transform_point(
        typename bg::select_most_precise
            <
                typename bg::coordinate_type<P1>::type,
                double
            >::type value)
{
    P1 p1;
    bg::set<0>(p1, 1);
    bg::set<1>(p1, 2);
    P2 p2;
    BOOST_CHECK(bg::transform(p1, p2));

    BOOST_CHECK_CLOSE(value * bg::get<0>(p1), bg::get<0>(p2), 0.001);
    BOOST_CHECK_CLOSE(value * bg::get<1>(p1), bg::get<1>(p2), 0.001);
}

template <typename P1, typename P2>
void test_transform_linestring()
{
    bg::model::linestring<P1> line1;
    line1.push_back(bg::make<P1>(1, 1));
    line1.push_back(bg::make<P1>(2, 2));
    bg::model::linestring<P2> line2;
    BOOST_CHECK(bg::transform(line1, line2));
    BOOST_CHECK_EQUAL(line1.size(), line2.size());

    std::ostringstream out1, out2;
    out1 << bg::wkt(line1);
    out2 << bg::wkt(line2);
    BOOST_CHECK_EQUAL(out1.str(), out1.str());
}


template <typename P1, typename P2>
void test_all(double value = 1.0)
{
    test_transform_point<P1, P2>(value);
    test_transform_linestring<P1, P2>();
}

template <typename T, typename DegreeOrRadian>
void test_transformations(double phi, double theta, double r)
{
    typedef bg::model::point<T, 3, bg::cs::cartesian> cartesian_type;
    cartesian_type p;

    // 1: using spherical coordinates
    {
        typedef bg::model::point<T, 3, bg::cs::spherical<DegreeOrRadian> >  spherical_type;
        spherical_type sph1;
        assign_values(sph1, phi, theta, r);
        BOOST_CHECK(transform(sph1, p));

        spherical_type sph2;
        BOOST_CHECK(transform(p, sph2));

        BOOST_CHECK_CLOSE(bg::get<0>(sph1), bg::get<0>(sph2), 0.001);
        BOOST_CHECK_CLOSE(bg::get<1>(sph1), bg::get<1>(sph2), 0.001);

        //std::cout << dsv(p) << std::endl;
        //std::cout << dsv(sph2) << std::endl;
    }

    // 2: using spherical coordinates on unit sphere
    {
        typedef bg::model::point<T, 2, bg::cs::spherical<DegreeOrRadian> >  spherical_type;
        spherical_type sph1, sph2;
        assign_values(sph1, phi, theta);
        BOOST_CHECK(transform(sph1, p));
        BOOST_CHECK(transform(p, sph2));

        BOOST_CHECK_CLOSE(bg::get<0>(sph1), bg::get<0>(sph2), 0.001);
        BOOST_CHECK_CLOSE(bg::get<1>(sph1), bg::get<1>(sph2), 0.001);

        //std::cout << dsv(sph1) << " " << dsv(p) << " " << dsv(sph2) << std::endl;
    }
}

int test_main(int, char* [])
{
    typedef bg::model::d2::point_xy<double > P;
    test_all<P, P>();
    test_all<bg::model::d2::point_xy<int>, bg::model::d2::point_xy<float> >();

    test_all<bg::model::point<double, 2, bg::cs::spherical<bg::degree> >,
        bg::model::point<double, 2, bg::cs::spherical<bg::radian> > >(bg::math::d2r);
    test_all<bg::model::point<double, 2, bg::cs::spherical<bg::radian> >,
        bg::model::point<double, 2, bg::cs::spherical<bg::degree> > >(bg::math::r2d);

    test_all<bg::model::point<int, 2, bg::cs::spherical<bg::degree> >,
        bg::model::point<float, 2, bg::cs::spherical<bg::radian> > >(bg::math::d2r);

    test_transformations<float, bg::degree>(4, 52, 1);
    test_transformations<double, bg::degree>(4, 52, 1);

    test_transformations<float, bg::radian>(3 * bg::math::d2r, 51 * bg::math::d2r, 1);
    test_transformations<double, bg::radian>(3 * bg::math::d2r, 51 * bg::math::d2r, 1);

#if defined(HAVE_TTMATH)
    typedef bg::model::d2::point_xy<ttmath_big > PT;
    test_all<PT, PT>();
    test_transformations<ttmath_big, bg::degree>(4, 52, 1);
    test_transformations<ttmath_big, bg::radian>(3 * bg::math::d2r, 51 * bg::math::d2r, 1);
#endif

    return 0;
}
