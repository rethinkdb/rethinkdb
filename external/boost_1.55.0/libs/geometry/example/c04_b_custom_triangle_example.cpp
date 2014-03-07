// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Custom triangle template Example

#include <iostream>

#include <boost/array.hpp>
#include <boost/tuple/tuple.hpp>

#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/centroid.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <boost/geometry/geometries/register/ring.hpp>
#include <boost/geometry/strategies/strategies.hpp>
#include <boost/geometry/io/dsv/write.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)


template <typename P>
struct triangle : public boost::array<P, 3>
{
};


// Register triangle<P> as a ring
BOOST_GEOMETRY_REGISTER_RING_TEMPLATED(triangle)


namespace boost { namespace geometry { namespace dispatch {

// Specializations of area dispatch structure, implement algorithm
template<typename Point>
struct area<triangle<Point>, ring_tag>
{
    template <typename Strategy>
    static inline double apply(triangle<Point> const& t, Strategy const&)
    {
        return 0.5  * ((get<0>(t[1]) - get<0>(t[0])) * (get<1>(t[2]) - get<1>(t[0]))
                     - (get<0>(t[2]) - get<0>(t[0])) * (get<1>(t[1]) - get<1>(t[0])));
    }
};

}}} // namespace boost::geometry::dispatch


int main()
{
    //triangle<boost::geometry::point_xy<double> > t;
    triangle<boost::tuple<double, double> > t;
    t[0] = boost::make_tuple(0, 0);
    t[1] = boost::make_tuple(5, 0);
    t[2] = boost::make_tuple(2.5, 2.5);

    std::cout << "Triangle: " << boost::geometry::dsv(t) << std::endl;
    std::cout << "Area: " << boost::geometry::area(t) << std::endl;

    //boost::geometry::point_xy<double> c;
    boost::tuple<double, double> c;
    boost::geometry::centroid(t, c);
    std::cout << "Centroid: " << boost::geometry::dsv(c) << std::endl;

    return 0;
}
