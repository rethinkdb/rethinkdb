// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_INTERSECTION_HPP
#define BOOST_GEOMETRY_STRATEGIES_INTERSECTION_HPP

#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/geometries/segment.hpp>

#include <boost/geometry/policies/relate/intersection_points.hpp>
#include <boost/geometry/policies/relate/direction.hpp>
#include <boost/geometry/policies/relate/tupled.hpp>

#include <boost/geometry/strategies/side.hpp>
#include <boost/geometry/strategies/intersection_result.hpp>

#include <boost/geometry/strategies/cartesian/cart_intersect.hpp>


namespace boost { namespace geometry
{


// The intersection strategy is a "compound strategy",
// it contains a segment-intersection-strategy
// and a side-strategy
/*!
\tparam CalculationType \tparam_calculation
*/
template
<
    typename Tag,
    typename Geometry1,
    typename Geometry2,
    typename IntersectionPoint,
    typename CalculationType = void
>
struct strategy_intersection
{
private :
    typedef typename geometry::point_type<Geometry1>::type point1_type;
    typedef typename geometry::point_type<Geometry2>::type point2_type;
    typedef typename model::referring_segment<point1_type const> segment1_type;
    typedef typename model::referring_segment<point2_type const> segment2_type;

    typedef segment_intersection_points
        <
            IntersectionPoint
        > ip_type;

public:
    typedef strategy::intersection::relate_cartesian_segments
            <
                policies::relate::segments_tupled
                    <
                        policies::relate::segments_intersection_points
                            <
                                segment1_type,
                                segment2_type,
                                ip_type,
                                CalculationType
                            > ,
                        policies::relate::segments_direction
                            <
                                segment1_type,
                                segment2_type,
                                CalculationType
                            >,
                        CalculationType
                    >,
                CalculationType
            > segment_intersection_strategy_type;

    typedef typename strategy::side::services::default_strategy
        <
            Tag,
            CalculationType
        >::type side_strategy_type;
};




}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_INTERSECTION_HPP
