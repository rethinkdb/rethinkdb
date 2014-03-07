// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <geometry_test_common.hpp>


#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/adapted/boost_polygon/point.hpp>
#include <boost/geometry/geometries/adapted/boost_polygon/box.hpp>
#include <boost/geometry/geometries/adapted/boost_polygon/ring.hpp>
#include <boost/geometry/geometries/adapted/boost_polygon/polygon.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>

#include<iostream>

template <typename T>
void test_overlay_using_boost_polygon(std::string const& case_id, std::string const& wkt_p, std::string const& wkt_q)
{
    typedef boost::polygon::polygon_set_data<T> polygon_set;

    polygon_set p, q;
    polygon_set out_i, out_u;

    {
        // Read polygons, conveniently using Boost.Geometry
        typedef boost::geometry::model::multi_polygon
            <
                boost::polygon::polygon_with_holes_data<T>
            > mp_type;
        mp_type mp, mq;

        bg::read_wkt(wkt_p, mp);
        bg::read_wkt(wkt_q, mq);

        p.insert(mp.begin(), mp.end());
        q.insert(mq.begin(), mq.end());
    }

    {
        using namespace boost::polygon::operators;
        out_i = p & q;
        out_u = p | q;
    }

    double area_p = boost::polygon::area(p);
    double area_q = boost::polygon::area(q);
    double area_i = boost::polygon::area(out_i);
    double area_u = boost::polygon::area(out_u);

    double sum = area_p + area_q - area_u - area_i;
    BOOST_CHECK_MESSAGE(abs(sum) < 0.001,
        "Overlay error\n"
            << "Boost.Polygon " << case_id
            << " area p: " << area_p
            << " area q: " << area_q
            << " area i: " << area_i
            << " area u: " << area_u
            << " sum: " << sum
         );
}


template <typename T>
void test_overlay_using_boost_geometry(std::string const& case_id, std::string const& wkt_p, std::string const& wkt_q)
{
    typedef boost::geometry::model::multi_polygon
        <
            boost::geometry::model::polygon
                <
                    boost::geometry::model::d2::point_xy<T>
                >
        > mp_type;

    // Read it using Boost.Geometry
    mp_type p, q, out_i, out_u;

    boost::geometry::read_wkt(wkt_p, p);
    boost::geometry::read_wkt(wkt_q, q);

    boost::geometry::intersection(p, q, out_i);
    boost::geometry::union_(p, q, out_u);

    double area_p = boost::geometry::area(p);
    double area_q = boost::geometry::area(q);
    double area_i = boost::geometry::area(out_i);
    double area_u = boost::geometry::area(out_u);

    double sum = area_p + area_q - area_u - area_i;
    BOOST_CHECK_MESSAGE(abs(sum) < 0.001,
        "Overlay error\n"
            << "Boost.Geometry " << case_id
            << " area p: " << area_p
            << " area q: " << area_q
            << " area i: " << area_i
            << " area u: " << area_u
            << " sum: " << sum
        );
}

template <typename T>
void test_overlay(std::string const& case_id, std::string const& wkt_p, std::string const& wkt_q)
{
    test_overlay_using_boost_polygon<T>(case_id, wkt_p, wkt_q);
    test_overlay_using_boost_geometry<T>(case_id, wkt_p, wkt_q);
}


int test_main(int, char* [])
{
    test_overlay<int>("case 1", "MULTIPOLYGON(((100 900,0 800,100 800,100 900)),((200 700,100 800,100 700,200 700)),((500 400,400 400,400 300,500 400)),((600 300,500 200,600 200,600 300)),((600 700,500 800,500 700,600 700)),((700 500,600 500,600 400,700 500)),((900 300,800 400,800 300,900 300)))",
        "MULTIPOLYGON(((200 900,100 1000,100 800,200 800,200 900)),((400 500,300 600,300 500,400 500)),((500 900,400 800,500 800,500 900)),((600 800,500 700,600 700,600 800)),((700 500,600 400,700 400,700 500)),((1000 1000,900 900,1000 900,1000 1000)))");
    test_overlay<int>("case 2", "MULTIPOLYGON(((200 400,100 400,100 300,200 400)),((300 100,200 100,200 0,300 0,300 100)),((600 700,500 700,500 600,600 700)),((700 300,600 300,600 200,700 300)),((800 500,700 600,700 500,800 500)),((900 800,800 700,900 700,900 800)),((1000 200,900 100,1000 100,1000 200)),((1000 800,900 900,900 800,1000 800)))",
        "MULTIPOLYGON(((200 800,100 800,100 700,200 700,200 800)),((400 200,300 100,400 100,400 200)),((400 800,300 700,400 700,400 800)),((700 100,600 0,700 0,700 100)),((700 200,600 200,600 100,700 200)),((900 200,800 200,800 0,900 0,900 200)),((1000 300,900 200,1000 200,1000 300)))");
    return 0;
}
