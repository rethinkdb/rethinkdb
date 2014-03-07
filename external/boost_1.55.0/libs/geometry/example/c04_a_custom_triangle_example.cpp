// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Custom Triangle Example

#include <iostream>

#include <boost/array.hpp>

#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/centroid.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/register/ring.hpp>
#include <boost/geometry/strategies/strategies.hpp>
#include <boost/geometry/io/dsv/write.hpp>


struct triangle : public boost::array<boost::geometry::model::d2::point_xy<double>, 4>
{
    inline void close()
    {
        (*this)[3] = (*this)[0];
    }
};


// Register triangle as a ring
BOOST_GEOMETRY_REGISTER_RING(triangle)


// Specializations of algorithms, where useful. If not specialized the default ones
// (for linear rings) will be used for triangle. Which is OK as long as the triangle
// is closed, that means, has 4 points (the last one being the first).
namespace boost { namespace geometry {

template<>
inline double area<triangle>(const triangle& t)
{
    /*         C
              / \
             /   \
            A-----B

           ((Bx - Ax) * (Cy - Ay)) - ((Cx - Ax) * (By - Ay))
           -------------------------------------------------
                                   2
    */

    return 0.5 * ((t[1].x() - t[0].x()) * (t[2].y() - t[0].y())
                - (t[2].x() - t[0].x()) * (t[1].y() - t[0].y()));
}

}} // namespace boost::geometry

int main()
{
    triangle t;

    t[0].x(0);
    t[0].y(0);
    t[1].x(5);
    t[1].y(0);
    t[2].x(2.5);
    t[2].y(2.5);

    t.close();

    std::cout << "Triangle: " << boost::geometry::dsv(t) << std::endl;
    std::cout << "Area: " << boost::geometry::area(t) << std::endl;

    boost::geometry::model::d2::point_xy<double> c;
    boost::geometry::centroid(t, c);
    std::cout << "Centroid: " << boost::geometry::dsv(c) << std::endl;

    return 0;
}
