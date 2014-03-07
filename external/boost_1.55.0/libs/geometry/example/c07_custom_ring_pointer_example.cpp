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

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/register/ring.hpp>

BOOST_GEOMETRY_REGISTER_RING_TEMPLATED(std::vector)


// Sample point, having x/y
struct my_point
{
    my_point(double a = 0, double b = 0)
        : x(a), y(b)
    {}
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
    typedef std::vector<my_point*> ring_type;

    ring_type a, b;

    a.push_back(new my_point(0, 1));
    a.push_back(new my_point(2, 5));
    a.push_back(new my_point(5, 3));
    a.push_back(new my_point(0, 1));

    b.push_back(new my_point(3, 0));
    b.push_back(new my_point(0, 3));
    b.push_back(new my_point(4, 5));
    b.push_back(new my_point(3, 0));

    double aa = boost::geometry::area(a);
    double ab = boost::geometry::area(b);

    std::cout << "a: " << aa << std::endl;
    std::cout << "b: " << ab << std::endl;

    // This will NOT work because would need dynamicly allocating memory for point* in algorithms:
    //std::vector<ring_type> unioned;
    //boost::geometry::union<ring_type>(a, b, unioned);

    // BEGIN TODO
    // This compiles (and once worked) using pointers, but has to be fixed or deprecated
    // The problem is now the cart_intersect/side where a temporary point is generated
    //typedef boost::geometry::model::ring<boost::geometry::model::d2::point_xy<double> > ring_2d;
    //std::vector<ring_2d> unioned;
    //std::vector<ring_2d> intersected;

    //boost::geometry::intersection(a, b, intersected);
    //boost::geometry::union_(a, b, unioned);

    //double ai = 0, au = 0;
    //BOOST_FOREACH(ring_2d const& ring, intersected)
    //{
    //    ai += boost::geometry::area(ring);
    //}
    //BOOST_FOREACH(ring_2d const& ring, unioned)
    //{
    //    au += boost::geometry::area(ring);
    //}

    //std::cout << "a: " << aa << std::endl;
    //std::cout << "b: " << ab << std::endl;
    //std::cout << "a & b: " << ai << std::endl;
    //std::cout << "a | b: " << au << std::endl;
    //std::cout << "a + b - (a & b): " << (aa + ab - ai) << std::endl;
    // END TODO


    // free
    BOOST_FOREACH(my_point* p, a)
    {
        delete p;
    }

    BOOST_FOREACH(my_point* p, b)
    {
        delete p;
    }

    return 0;
}
