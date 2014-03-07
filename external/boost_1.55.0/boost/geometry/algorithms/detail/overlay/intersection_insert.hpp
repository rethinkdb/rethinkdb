// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_INTERSECTION_INSERT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_INTERSECTION_INSERT_HPP


#include <cstddef>

#include <boost/mpl/if.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/range/metafunctions.hpp>


#include <boost/geometry/core/is_areal.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/reverse_dispatch.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/algorithms/detail/point_on_border.hpp>
#include <boost/geometry/algorithms/detail/overlay/clip_linestring.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_intersection_points.hpp>
#include <boost/geometry/algorithms/detail/overlay/overlay.hpp>
#include <boost/geometry/algorithms/detail/overlay/overlay_type.hpp>
#include <boost/geometry/algorithms/detail/overlay/follow.hpp>
#include <boost/geometry/views/segment_view.hpp>

#if defined(BOOST_GEOMETRY_DEBUG_FOLLOW)
#include <boost/foreach.hpp>
#endif

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace intersection
{

template <typename PointOut>
struct intersection_segment_segment_point
{
    template
    <
        typename Segment1, typename Segment2,
        typename OutputIterator, typename Strategy
    >
    static inline OutputIterator apply(Segment1 const& segment1,
            Segment2 const& segment2, OutputIterator out,
            Strategy const& )
    {
        typedef typename point_type<PointOut>::type point_type;

        // Get the intersection point (or two points)
        segment_intersection_points<point_type> is
            = strategy::intersection::relate_cartesian_segments
            <
                policies::relate::segments_intersection_points
                    <
                        Segment1,
                        Segment2,
                        segment_intersection_points<point_type>
                    >
            >::apply(segment1, segment2);

        for (std::size_t i = 0; i < is.count; i++)
        {
            PointOut p;
            geometry::convert(is.intersections[i], p);
            *out++ = p;
        }
        return out;
    }
};

template <typename PointOut>
struct intersection_linestring_linestring_point
{
    template
    <
        typename Linestring1, typename Linestring2,
        typename OutputIterator, typename Strategy
    >
    static inline OutputIterator apply(Linestring1 const& linestring1,
            Linestring2 const& linestring2, OutputIterator out,
            Strategy const& )
    {
        typedef typename point_type<PointOut>::type point_type;

        typedef detail::overlay::turn_info<point_type> turn_info;
        std::deque<turn_info> turns;

        geometry::get_intersection_points(linestring1, linestring2, turns);

        for (typename boost::range_iterator<std::deque<turn_info> const>::type
            it = boost::begin(turns); it != boost::end(turns); ++it)
        {
            PointOut p;
            geometry::convert(it->point, p);
            *out++ = p;
        }
        return out;
    }
};

/*!
\brief Version of linestring with an areal feature (polygon or multipolygon)
*/
template
<
    bool ReverseAreal,
    typename LineStringOut,
    overlay_type OverlayType
>
struct intersection_of_linestring_with_areal
{
#if defined(BOOST_GEOMETRY_DEBUG_FOLLOW)
        template <typename Turn, typename Operation>
        static inline void debug_follow(Turn const& turn, Operation op, 
                    int index)
        {
            std::cout << index
                << " at " << op.seg_id
                << " meth: " << method_char(turn.method)
                << " op: " << operation_char(op.operation)
                << " vis: " << visited_char(op.visited)
                << " of:  " << operation_char(turn.operations[0].operation)
                << operation_char(turn.operations[1].operation)
                << " " << geometry::wkt(turn.point)
                << std::endl;
        }
#endif

    template
    <
        typename LineString, typename Areal,
        typename OutputIterator, typename Strategy
    >
    static inline OutputIterator apply(LineString const& linestring, Areal const& areal,
            OutputIterator out,
            Strategy const& )
    {
        if (boost::size(linestring) == 0)
        {
            return out;
        }

        typedef detail::overlay::follow
                <
                    LineStringOut,
                    LineString,
                    Areal,
                    OverlayType
                > follower;

        typedef typename point_type<LineStringOut>::type point_type;

        typedef detail::overlay::traversal_turn_info<point_type> turn_info;
        std::deque<turn_info> turns;

        detail::get_turns::no_interrupt_policy policy;
        geometry::get_turns
            <
                false,
                (OverlayType == overlay_intersection ? ReverseAreal : !ReverseAreal),
                detail::overlay::calculate_distance_policy
            >(linestring, areal, turns, policy);

        if (turns.empty())
        {
            // No intersection points, it is either completely 
            // inside (interior + borders)
            // or completely outside

            // Use border point (on a segment) to check this
            // (because turn points might skip some cases)
            point_type border_point;
            if (! geometry::point_on_border(border_point, linestring, true))
            {
                return out;
            }


            if (follower::included(border_point, areal))
            {
                LineStringOut copy;
                geometry::convert(linestring, copy);
                *out++ = copy;
            }
            return out;
        }

#if defined(BOOST_GEOMETRY_DEBUG_FOLLOW)
        int index = 0;
        BOOST_FOREACH(turn_info const& turn, turns)
        {
            debug_follow(turn, turn.operations[0], index++);
        }
#endif

        return follower::apply
                (
                    linestring, areal,
                    geometry::detail::overlay::operation_intersection,
                    turns, out
                );
    }
};


}} // namespace detail::intersection
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    // real types
    typename Geometry1, typename Geometry2,
    typename GeometryOut,
    overlay_type OverlayType,
    // orientation
    bool Reverse1 = detail::overlay::do_reverse<geometry::point_order<Geometry1>::value>::value,
    bool Reverse2 = detail::overlay::do_reverse<geometry::point_order<Geometry2>::value>::value,
    bool ReverseOut = detail::overlay::do_reverse<geometry::point_order<GeometryOut>::value>::value,
    // tag dispatching:
    typename TagIn1 = typename geometry::tag<Geometry1>::type,
    typename TagIn2 = typename geometry::tag<Geometry2>::type,
    typename TagOut = typename geometry::tag<GeometryOut>::type,
    // metafunction finetuning helpers:
    bool Areal1 = geometry::is_areal<Geometry1>::value,
    bool Areal2 = geometry::is_areal<Geometry2>::value,
    bool ArealOut = geometry::is_areal<GeometryOut>::value
>
struct intersection_insert
{
    BOOST_MPL_ASSERT_MSG
        (
            false, NOT_OR_NOT_YET_IMPLEMENTED_FOR_THIS_GEOMETRY_TYPES_OR_ORIENTATIONS
            , (types<Geometry1, Geometry2, GeometryOut>)
        );
};


template
<
    typename Geometry1, typename Geometry2,
    typename GeometryOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2, bool ReverseOut,
    typename TagIn1, typename TagIn2, typename TagOut
>
struct intersection_insert
    <
        Geometry1, Geometry2,
        GeometryOut,
        OverlayType,
        Reverse1, Reverse2, ReverseOut,
        TagIn1, TagIn2, TagOut,
        true, true, true
    > : detail::overlay::overlay
        <Geometry1, Geometry2, Reverse1, Reverse2, ReverseOut, GeometryOut, OverlayType>
{};


// Any areal type with box:
template
<
    typename Geometry, typename Box,
    typename GeometryOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2, bool ReverseOut,
    typename TagIn, typename TagOut
>
struct intersection_insert
    <
        Geometry, Box,
        GeometryOut,
        OverlayType,
        Reverse1, Reverse2, ReverseOut,
        TagIn, box_tag, TagOut,
        true, true, true
    > : detail::overlay::overlay
        <Geometry, Box, Reverse1, Reverse2, ReverseOut, GeometryOut, OverlayType>
{};


template
<
    typename Segment1, typename Segment2,
    typename GeometryOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2, bool ReverseOut
>
struct intersection_insert
    <
        Segment1, Segment2,
        GeometryOut,
        OverlayType,
        Reverse1, Reverse2, ReverseOut,
        segment_tag, segment_tag, point_tag,
        false, false, false
    > : detail::intersection::intersection_segment_segment_point<GeometryOut>
{};


template
<
    typename Linestring1, typename Linestring2,
    typename GeometryOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2, bool ReverseOut
>
struct intersection_insert
    <
        Linestring1, Linestring2,
        GeometryOut,
        OverlayType,
        Reverse1, Reverse2, ReverseOut,
        linestring_tag, linestring_tag, point_tag,
        false, false, false
    > : detail::intersection::intersection_linestring_linestring_point<GeometryOut>
{};


template
<
    typename Linestring, typename Box,
    typename GeometryOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2, bool ReverseOut
>
struct intersection_insert
    <
        Linestring, Box,
        GeometryOut,
        OverlayType,
        Reverse1, Reverse2, ReverseOut,
        linestring_tag, box_tag, linestring_tag,
        false, true, false
    >
{
    template <typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Linestring const& linestring,
            Box const& box, OutputIterator out, Strategy const& )
    {
        typedef typename point_type<GeometryOut>::type point_type;
        strategy::intersection::liang_barsky<Box, point_type> lb_strategy;
        return detail::intersection::clip_range_with_box
            <GeometryOut>(box, linestring, out, lb_strategy);
    }
};


template
<
    typename Linestring, typename Polygon,
    typename GeometryOut,
    overlay_type OverlayType,
    bool ReverseLinestring, bool ReversePolygon, bool ReverseOut
>
struct intersection_insert
    <
        Linestring, Polygon,
        GeometryOut,
        OverlayType,
        ReverseLinestring, ReversePolygon, ReverseOut,
        linestring_tag, polygon_tag, linestring_tag,
        false, true, false
    > : detail::intersection::intersection_of_linestring_with_areal
            <
                ReversePolygon,
                GeometryOut,
                OverlayType
            >
{};


template
<
    typename Linestring, typename Ring,
    typename GeometryOut,
    overlay_type OverlayType,
    bool ReverseLinestring, bool ReverseRing, bool ReverseOut
>
struct intersection_insert
    <
        Linestring, Ring,
        GeometryOut,
        OverlayType,
        ReverseLinestring, ReverseRing, ReverseOut,
        linestring_tag, ring_tag, linestring_tag,
        false, true, false
    > : detail::intersection::intersection_of_linestring_with_areal
            <
                ReverseRing,
                GeometryOut,
                OverlayType
            >
{};

template
<
    typename Segment, typename Box,
    typename GeometryOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2, bool ReverseOut
>
struct intersection_insert
    <
        Segment, Box,
        GeometryOut,
        OverlayType,
        Reverse1, Reverse2, ReverseOut,
        segment_tag, box_tag, linestring_tag,
        false, true, false
    >
{
    template <typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Segment const& segment,
            Box const& box, OutputIterator out, Strategy const& )
    {
        geometry::segment_view<Segment> range(segment);

        typedef typename point_type<GeometryOut>::type point_type;
        strategy::intersection::liang_barsky<Box, point_type> lb_strategy;
        return detail::intersection::clip_range_with_box
            <GeometryOut>(box, range, out, lb_strategy);
    }
};

template
<
    typename Geometry1, typename Geometry2,
    typename PointOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2, bool ReverseOut,
    typename Tag1, typename Tag2,
    bool Areal1, bool Areal2
>
struct intersection_insert
    <
        Geometry1, Geometry2,
        PointOut,
        OverlayType,
        Reverse1, Reverse2, ReverseOut,
        Tag1, Tag2, point_tag,
        Areal1, Areal2, false
    >
{
    template <typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Geometry1 const& geometry1,
            Geometry2 const& geometry2, OutputIterator out, Strategy const& )
    {

        typedef detail::overlay::turn_info<PointOut> turn_info;
        std::vector<turn_info> turns;

        detail::get_turns::no_interrupt_policy policy;
        geometry::get_turns
            <
                false, false, detail::overlay::assign_null_policy
            >(geometry1, geometry2, turns, policy);
        for (typename std::vector<turn_info>::const_iterator it
            = turns.begin(); it != turns.end(); ++it)
        {
            *out++ = it->point;
        }

        return out;
    }
};


template
<
    typename Geometry1, typename Geometry2, typename GeometryOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2, bool ReverseOut
>
struct intersection_insert_reversed
{
    template <typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Geometry1 const& g1,
                Geometry2 const& g2, OutputIterator out,
                Strategy const& strategy)
    {
        return intersection_insert
            <
                Geometry2, Geometry1, GeometryOut,
                OverlayType,
                Reverse2, Reverse1, ReverseOut
            >::apply(g2, g1, out, strategy);
    }
};



} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace intersection
{


template
<
    typename GeometryOut,
    bool ReverseSecond,
    overlay_type OverlayType,
    typename Geometry1, typename Geometry2,
    typename OutputIterator,
    typename Strategy
>
inline OutputIterator insert(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            OutputIterator out,
            Strategy const& strategy)
{
    return boost::mpl::if_c
        <
            geometry::reverse_dispatch<Geometry1, Geometry2>::type::value,
            geometry::dispatch::intersection_insert_reversed
            <
                Geometry1, Geometry2,
                GeometryOut,
                OverlayType,
                overlay::do_reverse<geometry::point_order<Geometry1>::value>::value,
                overlay::do_reverse<geometry::point_order<Geometry2>::value, ReverseSecond>::value,
                overlay::do_reverse<geometry::point_order<GeometryOut>::value>::value
            >,
            geometry::dispatch::intersection_insert
            <
                Geometry1, Geometry2,
                GeometryOut,
                OverlayType,
                geometry::detail::overlay::do_reverse<geometry::point_order<Geometry1>::value>::value,
                geometry::detail::overlay::do_reverse<geometry::point_order<Geometry2>::value, ReverseSecond>::value
            >
        >::type::apply(geometry1, geometry2, out, strategy);
}


/*!
\brief \brief_calc2{intersection} \brief_strategy
\ingroup intersection
\details \details_calc2{intersection_insert, spatial set theoretic intersection}
    \brief_strategy. \details_insert{intersection}
\tparam GeometryOut \tparam_geometry{\p_l_or_c}
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam OutputIterator \tparam_out{\p_l_or_c}
\tparam Strategy \tparam_strategy_overlay
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param out \param_out{intersection}
\param strategy \param_strategy{intersection}
\return \return_out

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/intersection.qbk]}
*/
template
<
    typename GeometryOut,
    typename Geometry1,
    typename Geometry2,
    typename OutputIterator,
    typename Strategy
>
inline OutputIterator intersection_insert(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            OutputIterator out,
            Strategy const& strategy)
{
    concept::check<Geometry1 const>();
    concept::check<Geometry2 const>();

    return detail::intersection::insert
        <
            GeometryOut, false, overlay_intersection
        >(geometry1, geometry2, out, strategy);
}


/*!
\brief \brief_calc2{intersection}
\ingroup intersection
\details \details_calc2{intersection_insert, spatial set theoretic intersection}.
    \details_insert{intersection}
\tparam GeometryOut \tparam_geometry{\p_l_or_c}
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam OutputIterator \tparam_out{\p_l_or_c}
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param out \param_out{intersection}
\return \return_out

\qbk{[include reference/algorithms/intersection.qbk]}
*/
template
<
    typename GeometryOut,
    typename Geometry1,
    typename Geometry2,
    typename OutputIterator
>
inline OutputIterator intersection_insert(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            OutputIterator out)
{
    concept::check<Geometry1 const>();
    concept::check<Geometry2 const>();

    typedef strategy_intersection
        <
            typename cs_tag<GeometryOut>::type,
            Geometry1,
            Geometry2,
            typename geometry::point_type<GeometryOut>::type
        > strategy;

    return intersection_insert<GeometryOut>(geometry1, geometry2, out,
                strategy());
}

}} // namespace detail::intersection
#endif // DOXYGEN_NO_DETAIL



}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_INTERSECTION_INSERT_HPP
