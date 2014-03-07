// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_COVERED_BY_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_COVERED_BY_HPP


#include <boost/geometry/algorithms/covered_by.hpp>
#include <boost/geometry/multi/core/closure.hpp>
#include <boost/geometry/multi/core/point_order.hpp>
#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>
#include <boost/geometry/multi/algorithms/within.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Point, typename MultiPolygon>
struct covered_by<Point, MultiPolygon, point_tag, multi_polygon_tag>
{
    template <typename Strategy>
    static inline bool apply(Point const& point, 
                MultiPolygon const& multi_polygon, Strategy const& strategy)
    {
        return detail::within::geometry_multi_within_code
            <
                Point,
                MultiPolygon,
                Strategy,
                detail::within::point_in_polygon
                        <
                            Point,
                            typename boost::range_value<MultiPolygon>::type,
                            order_as_direction
                                <
                                    geometry::point_order<MultiPolygon>::value
                                >::value,
                            geometry::closure<MultiPolygon>::value,
                            Strategy
                        >
            >::apply(point, multi_polygon, strategy) >= 0;
    }
};


} // namespace dispatch


#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_COVERED_BY_HPP
