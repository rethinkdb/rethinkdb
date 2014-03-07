// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// geometry::num_points does not accept the ring_proxy for adaptation, like
// centroid and probably many more algorithms. TODO: fix that. Until then we
// define it such that num_points is not called.
#define BOOST_GEOMETRY_EMPTY_INPUT_NO_THROW

#include <geometry_test_common.hpp>


#include <boost/geometry/geometry.hpp>

#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/ring.hpp>

#include <boost/geometry/geometries/adapted/boost_polygon/point.hpp>
#include <boost/geometry/geometries/adapted/boost_polygon/box.hpp>
#include <boost/geometry/geometries/adapted/boost_polygon/ring.hpp>
#include <boost/geometry/geometries/adapted/boost_polygon/polygon.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>

#include <iostream>

template <typename T>
void fill_polygon_with_two_holes(boost::polygon::polygon_with_holes_data<T>& boost_polygon_polygon)
{
    std::vector<boost::polygon::point_data<T> > point_vector;
    point_vector.push_back(boost::polygon::point_data<T>(0, 0));
    point_vector.push_back(boost::polygon::point_data<T>(0, 10));
    point_vector.push_back(boost::polygon::point_data<T>(10, 10));
    point_vector.push_back(boost::polygon::point_data<T>(10, 0));
    point_vector.push_back(boost::polygon::point_data<T>(0, 0));
    boost_polygon_polygon.set(point_vector.begin(), point_vector.end());


    std::vector<boost::polygon::polygon_data<T> > holes;
    holes.resize(2);

    {
        std::vector<boost::polygon::point_data<T> > point_vector;
        point_vector.push_back(boost::polygon::point_data<T>(1, 1));
        point_vector.push_back(boost::polygon::point_data<T>(2, 1));
        point_vector.push_back(boost::polygon::point_data<T>(2, 2));
        point_vector.push_back(boost::polygon::point_data<T>(1, 2));
        point_vector.push_back(boost::polygon::point_data<T>(1, 1));
        holes[0].set(point_vector.begin(), point_vector.end());
    }

    {
        std::vector<boost::polygon::point_data<T> > point_vector;
        point_vector.push_back(boost::polygon::point_data<T>(3, 3));
        point_vector.push_back(boost::polygon::point_data<T>(4, 3));
        point_vector.push_back(boost::polygon::point_data<T>(4, 4));
        point_vector.push_back(boost::polygon::point_data<T>(3, 4));
        point_vector.push_back(boost::polygon::point_data<T>(3, 3));
        holes[1].set(point_vector.begin(), point_vector.end());
    }
    boost_polygon_polygon.set_holes(holes.begin(), holes.end());
}


template <typename T>
void test_coordinate_type()
{
    // 1a: Check if Boost.Polygon's point fulfills Boost.Geometry's point concept
    bg::concept::check<boost::polygon::point_data<T> >();

    // 1b: use a Boost.Polygon point in Boost.Geometry, calc. distance with two point types
    boost::polygon::point_data<T> boost_polygon_point(1, 2);

    typedef bg::model::point<T, 2, bg::cs::cartesian> bg_point_type;
    bg_point_type boost_geometry_point(3, 4);
    BOOST_CHECK_EQUAL(bg::distance(boost_polygon_point, boost_geometry_point),
                    2 * std::sqrt(2.0));

    // 2a: Check if Boost.Polygon's box fulfills Boost.Geometry's box concept
    bg::concept::check<boost::polygon::rectangle_data<T> >();

    // 2b: use a Boost.Polygon rectangle in Boost.Geometry, compare with boxes
    boost::polygon::rectangle_data<T> boost_polygon_box;
    bg::model::box<bg_point_type> boost_geometry_box;

    bg::assign_values(boost_polygon_box, 0, 1, 5, 6);
    bg::assign_values(boost_geometry_box, 0, 1, 5, 6);
    T boost_polygon_area = bg::area(boost_polygon_box);
    T boost_geometry_area = bg::area(boost_geometry_box);
    T boost_polygon_area_by_boost_polygon = boost::polygon::area(boost_polygon_box);
    BOOST_CHECK_EQUAL(boost_polygon_area, boost_geometry_area);
    BOOST_CHECK_EQUAL(boost_polygon_area, boost_polygon_area_by_boost_polygon);

    // 3a: Check if Boost.Polygon's polygon fulfills Boost.Geometry's ring concept
    bg::concept::check<boost::polygon::polygon_data<T> >();

    // 3b: use a Boost.Polygon polygon (ring)
    boost::polygon::polygon_data<T> boost_polygon_ring;
    {
        // Filling it is a two-step process using Boost.Polygon
        std::vector<boost::polygon::point_data<T> > point_vector;
        point_vector.push_back(boost::polygon::point_data<T>(0, 0));
        point_vector.push_back(boost::polygon::point_data<T>(0, 3));
        point_vector.push_back(boost::polygon::point_data<T>(4, 0));
        point_vector.push_back(boost::polygon::point_data<T>(0, 0));
        boost_polygon_ring.set(point_vector.begin(), point_vector.end());
    }

    // Boost-geometry ring
    bg::model::ring<bg_point_type> boost_geometry_ring;
    {
        boost_geometry_ring.push_back(bg_point_type(0, 0));
        boost_geometry_ring.push_back(bg_point_type(0, 3));
        boost_geometry_ring.push_back(bg_point_type(4, 0));
        boost_geometry_ring.push_back(bg_point_type(0, 0));
    }
    boost_polygon_area = bg::area(boost_polygon_ring);
    boost_geometry_area = bg::area(boost_geometry_ring);
    boost_polygon_area_by_boost_polygon = boost::polygon::area(boost_polygon_ring);
    BOOST_CHECK_EQUAL(boost_polygon_area, boost_geometry_area);
    BOOST_CHECK_EQUAL(boost_polygon_area, boost_polygon_area_by_boost_polygon);

    // Check mutable ring
    std::string wkt = "POLYGON((0 0,0 10,10 10,10 0,0 0))";
    bg::read_wkt(wkt, boost_polygon_ring);
    bg::read_wkt(wkt, boost_geometry_ring);
    boost_polygon_area = bg::area(boost_polygon_ring);
    boost_geometry_area = bg::area(boost_geometry_ring);
    boost_polygon_area_by_boost_polygon = boost::polygon::area(boost_polygon_ring);
    BOOST_CHECK_EQUAL(boost_polygon_area, boost_geometry_area);
    BOOST_CHECK_EQUAL(boost_polygon_area, boost_polygon_area_by_boost_polygon);

    // 4a: Boost.Polygon's polygon with holes
    boost::polygon::polygon_with_holes_data<T> boost_polygon_polygon;
    fill_polygon_with_two_holes(boost_polygon_polygon);

    // Using Boost.Polygon
    boost_polygon_area = bg::area(boost_polygon_polygon);
    boost_polygon_area_by_boost_polygon = boost::polygon::area(boost_polygon_polygon);
    BOOST_CHECK_EQUAL(boost_polygon_area, boost_polygon_area_by_boost_polygon);

    wkt = "POLYGON((0 0,0 10,10 10,10 0,0 0),(1 1,2 1,2 2,1 2,1 1),(3 3,4 3,4 4,3 4,3 3))";

    bg::model::polygon<bg_point_type> boost_geometry_polygon;
    bg::read_wkt(wkt, boost_geometry_polygon);

    boost_geometry_area = bg::area(boost_geometry_polygon);
    BOOST_CHECK_EQUAL(boost_polygon_area, boost_geometry_area);

    bg::clear(boost_polygon_polygon);
    bg::read_wkt(wkt, boost_polygon_polygon);
    boost_geometry_area = bg::area(boost_polygon_polygon);
    BOOST_CHECK_EQUAL(boost_polygon_area, boost_geometry_area);

    std::ostringstream out;
    out << bg::wkt(boost_polygon_polygon);
    BOOST_CHECK_EQUAL(wkt, out.str());

}

int test_main(int, char* [])
{
    test_coordinate_type<int>();
    //test_coordinate_type<float>(); // compiles, but "BOOST_CHECK_EQUAL" fails
    test_coordinate_type<double>();
    return 0;
}
