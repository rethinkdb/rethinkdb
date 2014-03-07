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

#include <boost/tuple/tuple.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/register/point.hpp>

#include "function_requiring_a_point.hpp"

struct point: public boost::tuple<float, float>
{
};

BOOST_GEOMETRY_REGISTER_POINT_2D(point, float, cs::cartesian, get<0>(), get<1>())

int main()
{
    point p1;
    point const p2 = point();
    test::function_requiring_a_point(p1, p2);
    return 0;
}
