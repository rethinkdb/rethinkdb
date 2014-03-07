// Boost.Geometry (aka GGL, Generic Geometry Library)
// Robustness Test
//
// Copyright (c) 2009-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_TEST_ROBUSTNESS_MAKE_SQUARE_POLYGON_HPP
#define BOOST_GEOMETRY_TEST_ROBUSTNESS_MAKE_SQUARE_POLYGON_HPP

#include <boost/geometry.hpp>

template <typename Polygon, typename Generator, typename Settings>
inline void make_square_polygon(Polygon& polygon, Generator& generator, Settings const& settings)
{
    using namespace boost::geometry;
    
    typedef typename point_type<Polygon>::type point_type;
    typedef typename coordinate_type<Polygon>::type coordinate_type;

    coordinate_type x, y;
    x = generator();
    y = generator();

    typename ring_type<Polygon>::type& ring = exterior_ring(polygon);

    point_type p;
    set<0>(p, x); set<1>(p, y);         append(ring, p);
    set<0>(p, x); set<1>(p, y + 1);     append(ring, p);
    set<0>(p, x + 1); set<1>(p, y + 1); append(ring, p);
    set<0>(p, x + 1); set<1>(p, y);     append(ring, p);
    set<0>(p, x); set<1>(p, y);         append(ring, p);

    if (settings.triangular)
    {
        // Remove a point, generator says which
        int c = generator() % 4;
        if (c >= 1 && c <= 3)
        {
            ring.erase(ring.begin() + c);
        }
    }
}

#endif // BOOST_GEOMETRY_TEST_ROBUSTNESS_MAKE_SQUARE_POLYGON_HPP
