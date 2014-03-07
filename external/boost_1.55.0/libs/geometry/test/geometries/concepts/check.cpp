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


#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>

struct ro_point
{
    float x, y;
};


struct rw_point
{
    float x, y;
};


namespace boost { namespace geometry { namespace traits {

template <> struct tag<ro_point> { typedef point_tag type; };
template <> struct coordinate_type<ro_point> { typedef float type; };
template <> struct coordinate_system<ro_point> { typedef bg::cs::cartesian type; };
template <> struct dimension<ro_point> { enum { value = 2 }; };

template <> struct access<ro_point, 0>
{
    static float get(ro_point const& p) { return p.x; }
};

template <> struct access<ro_point, 1>
{
    static float get(ro_point const& p) { return p.y; }
};




template <> struct tag<rw_point> { typedef point_tag type; };
template <> struct coordinate_type<rw_point> { typedef float type; };
template <> struct coordinate_system<rw_point> { typedef bg::cs::cartesian type; };
template <> struct dimension<rw_point> { enum { value = 2 }; };

template <> struct access<rw_point, 0>
{
    static float get(rw_point const& p) { return p.x; }
    static void set(rw_point& p, float value) { p.x = value; }
};

template <> struct access<rw_point, 1>
{
    static float get(rw_point const& p) { return p.y; }
    static void set(rw_point& p, float value) { p.y = value; }
};


}}} // namespace bg::traits


int main()
{
    bg::concept::check<const ro_point>();
    bg::concept::check<rw_point>();
}
