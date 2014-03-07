// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_POINT_ON_BORDER_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_POINT_ON_BORDER_HPP

#include <boost/range.hpp>

#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>
#include <boost/geometry/algorithms/detail/point_on_border.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace point_on_border
{


template
<
    typename Point,
    typename MultiGeometry,
    typename Policy
>
struct point_on_multi
{
    static inline bool apply(Point& point, MultiGeometry const& multi, bool midpoint)
    {
        // Take a point on the first multi-geometry
        // (i.e. the first that is not empty)
        for (typename boost::range_iterator
                <
                    MultiGeometry const
                >::type it = boost::begin(multi);
            it != boost::end(multi);
            ++it)
        {
            if (Policy::apply(point, *it, midpoint))
            {
                return true;
            }
        }
        return false;
    }
};




}} // namespace detail::point_on_border
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template<typename Point, typename Multi>
struct point_on_border<multi_polygon_tag, Point, Multi>
    : detail::point_on_border::point_on_multi
        <
            Point,
            Multi,
            detail::point_on_border::point_on_polygon
                <
                    Point,
                    typename boost::range_value<Multi>::type
                >
        >
{};



} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH



}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_POINT_ON_BORDER_HPP
