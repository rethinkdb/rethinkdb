// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Custom Polygon Example
#include <iostream>

#include <boost/geometry/geometry.hpp>

#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/ring.hpp>


struct my_point
{
    my_point(double an_x = 0, double an_y = 0)
        : x(an_x)
        , y(an_y)
    {}

    double x, y;
};

struct my_ring : std::deque<my_point>
{};

// Define a struct of a polygon, having always two holes
// (of course this can be implemented differently, usually
// with a vector or deque, but it is just an exampe)
struct my_polygon
{
    // required for a polygon: an outer ring...
    my_ring boundary;
    // ... and a Boost.Range compatible inner ring collection
    boost::array<my_ring, 2> holes;

    // just for the sample
    std::string name;

    my_polygon(std::string const& n = "") : name(n) {}
};


// We can conveniently use macro's to register point and ring
BOOST_GEOMETRY_REGISTER_POINT_2D(my_point, double, cs::cartesian, x, y)
BOOST_GEOMETRY_REGISTER_RING(my_ring)



// There is currently no registration macro for polygons
// and besides that a boost::array<T,N> in a macro would
// be very specific, so we show it "by hand":
namespace boost { namespace geometry { namespace traits
{

template<> struct tag<my_polygon> { typedef polygon_tag type; };
template<> struct ring_const_type<my_polygon> { typedef my_ring const& type; };
template<> struct ring_mutable_type<my_polygon> { typedef my_ring& type; };

template<> struct interior_const_type<my_polygon>
{
    typedef boost::array<my_ring, 2> const& type;
};

template<> struct interior_mutable_type<my_polygon>
{
    typedef boost::array<my_ring, 2>& type;
};

template<> struct exterior_ring<my_polygon>
{
    static my_ring& get(my_polygon& p)
    {
        return p.boundary;
    }

    static my_ring const& get(my_polygon const& p)
    {
        return p.boundary;
    }
};

template<> struct interior_rings<my_polygon>
{
    typedef boost::array<my_ring, 2> holes_type;

    static holes_type& get(my_polygon& p)
    {
        return p.holes;
    }

    static holes_type const& get(my_polygon const& p)
    {
        return p.holes;
    }
};

}}} // namespace boost::geometry::traits



int main()
{
    my_polygon p1("my polygon");

    // Fill it the my-way, triangle
    p1.boundary.push_back(my_point(2, 0));
    p1.boundary.push_back(my_point(1, 5));
    p1.boundary.push_back(my_point(7, 6));
    p1.boundary.push_back(my_point(2, 0));

    // Triangle
    p1.holes[0].push_back(my_point(2, 1));
    p1.holes[0].push_back(my_point(2.4, 2));
    p1.holes[0].push_back(my_point(1.9, 2));
    p1.holes[0].push_back(my_point(2, 1));

    // Box
    p1.holes[1].push_back(my_point(3, 3));
    p1.holes[1].push_back(my_point(4, 3));
    p1.holes[1].push_back(my_point(4, 4));
    p1.holes[1].push_back(my_point(3, 4));
    p1.holes[1].push_back(my_point(3, 3));

    std::cout << "Representation of " << p1.name << ": "
        << boost::geometry::dsv(p1) << std::endl;
    std::cout << "Area of " << p1.name << ": "
        << boost::geometry::area(p1) << std::endl;
    std::cout << "Perimeter of " << p1.name << ": "
        << boost::geometry::perimeter(p1) << std::endl;
    std::cout << "Centroid of " << p1.name << ": "
        << boost::geometry::dsv(boost::geometry::return_centroid<my_point>(p1)) << std::endl;

    return 0;
}
