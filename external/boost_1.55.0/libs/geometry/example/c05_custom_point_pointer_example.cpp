// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Custom pointer-to-point example

#include <iostream>

#include <boost/foreach.hpp>

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/algorithms/length.hpp>
#include <boost/geometry/algorithms/make.hpp>
#include <boost/geometry/algorithms/intersection.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/register/linestring.hpp>
#include <boost/geometry/strategies/strategies.hpp>

BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::vector)
BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::deque)

// Sample point, having x/y
struct my_point
{
    double x,y;
};


namespace boost { namespace geometry { namespace traits {

template<> struct tag<my_point>
{ typedef point_tag type; };

template<> struct coordinate_type<my_point>
{ typedef double type; };

template<> struct coordinate_system<my_point>
{ typedef cs::cartesian type; };

template<> struct dimension<my_point> : boost::mpl::int_<2> {};

template<>
struct access<my_point, 0>
{
    static double get(my_point const& p)
    {
        return p.x;
    }

    static void set(my_point& p, double const& value)
    {
        p.x = value;
    }
};

template<>
struct access<my_point, 1>
{
    static double get(my_point const& p)
    {
        return p.y;
    }

    static void set(my_point& p, double const& value)
    {
        p.y = value;
    }
};

}}} // namespace boost::geometry::traits



int main()
{
    typedef std::vector<my_point*> ln;
    ln myline;
    for (float i = 0.0; i < 10.0; i++)
    {
        my_point* p = new my_point;
        p->x = i;
        p->y = i + 1;
        myline.push_back(p);
    }

    std::cout << boost::geometry::length(myline) << std::endl;

    typedef boost::geometry::model::d2::point_xy<double> point_2d;
    typedef boost::geometry::model::box<point_2d> box_2d;
    box_2d cb(point_2d(1.5, 1.5), point_2d(4.5, 4.5));

    // This will NOT work because would need dynamicly allocating memory for point* in algorithms:
    // std::vector<ln> clipped;
    //boost::geometry::intersection(cb, myline, clipped);

    // This works because outputs to a normal struct point, no point*
    typedef boost::geometry::model::linestring<point_2d> linestring_2d;
    std::vector<linestring_2d> clipped;
    boost::geometry::strategy::intersection::liang_barsky<box_2d, point_2d> strategy;
    boost::geometry::detail::intersection::clip_range_with_box<linestring_2d>(cb,
                    myline, std::back_inserter(clipped), strategy);


    std::cout << boost::geometry::length(clipped.front()) << std::endl;

    // free
    BOOST_FOREACH(my_point* p, myline)
    {
        delete p;
    }

    return 0;
}
