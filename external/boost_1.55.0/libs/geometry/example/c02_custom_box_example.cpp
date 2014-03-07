// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Custom Box Example

#include <iostream>

#include <boost/geometry/algorithms/make.hpp>
#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/box.hpp>
#include <boost/geometry/strategies/strategies.hpp>
#include <boost/geometry/io/dsv/write.hpp>

struct my_point
{
    double x, y;
};

struct my_int_point
{
    int x, y;
};

struct my_box
{
    my_point ll, ur;
};

struct my_box_ltrb
{
    int left, top, right, bottom;
};

struct my_box_4
{
    double coors[4];
};

template <typename P>
struct my_box_t
{
    P ll, ur;
};

BOOST_GEOMETRY_REGISTER_POINT_2D(my_point, double, cs::cartesian, x, y)
BOOST_GEOMETRY_REGISTER_POINT_2D(my_int_point, int, cs::cartesian, x, y)
BOOST_GEOMETRY_REGISTER_BOX(my_box, my_point, ll, ur)
BOOST_GEOMETRY_REGISTER_BOX_TEMPLATED(my_box_t, ll, ur)
BOOST_GEOMETRY_REGISTER_BOX_2D_4VALUES(my_box_ltrb, my_int_point, left, top, right, bottom)
BOOST_GEOMETRY_REGISTER_BOX_2D_4VALUES(my_box_4, my_point, coors[0], coors[1], coors[2], coors[3])

int main()
{
    my_point p = boost::geometry::make<my_point>(3.5, 3.5);
    my_box b = boost::geometry::make<my_box>(0, 0, 2, 2);
    my_box_ltrb b1 = boost::geometry::make<my_box_ltrb>(0, 0, 3, 3);
    my_box_4 b4 = boost::geometry::make<my_box_4>(0, 0, 4, 4);
    my_box_t<my_point> bt = boost::geometry::make<my_box_t<my_point> >(0, 0, 5, 5);

    std::cout << boost::geometry::dsv(p) << " IN " << boost::geometry::dsv(b)
            << " : " << int(boost::geometry::within(p, b)) << std::endl;
    std::cout << boost::geometry::dsv(p) << " IN " << boost::geometry::dsv(b1)
            << " : " << int(boost::geometry::within(p, b1)) << std::endl;
    std::cout << boost::geometry::dsv(p) << " IN " << boost::geometry::dsv(b4)
            << " : " << int(boost::geometry::within(p, b4)) << std::endl;
    std::cout << boost::geometry::dsv(p) << " IN " << boost::geometry::dsv(bt)
            << " : " << int(boost::geometry::within(p, bt)) << std::endl;

    return 0;
}
