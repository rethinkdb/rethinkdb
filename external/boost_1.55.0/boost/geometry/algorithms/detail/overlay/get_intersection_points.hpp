// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_INTERSECTION_POINTS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_INTERSECTION_POINTS_HPP


#include <cstddef>

#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>

#include <boost/geometry/geometries/segment.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace get_intersection_points
{


template
<
    typename Point1,
    typename Point2,
    typename TurnInfo
>
struct get_turn_without_info
{
    typedef strategy_intersection
        <
            typename cs_tag<typename TurnInfo::point_type>::type,
            Point1,
            Point2,
            typename TurnInfo::point_type
        > si;

    typedef typename si::segment_intersection_strategy_type strategy;



    template <typename OutputIterator>
    static inline OutputIterator apply(
                Point1 const& pi, Point1 const& pj, Point1 const& pk,
                Point2 const& qi, Point2 const& qj, Point2 const& qk,
                TurnInfo const& ,
                OutputIterator out)
    {
        typedef model::referring_segment<Point1 const> segment_type1;
        typedef model::referring_segment<Point1 const> segment_type2;
        segment_type1 p1(pi, pj), p2(pj, pk);
        segment_type2 q1(qi, qj), q2(qj, qk);

        //
        typename strategy::return_type result = strategy::apply(p1, q1);

        for (std::size_t i = 0; i < result.template get<0>().count; i++)
        {

            TurnInfo tp;
            geometry::convert(result.template get<0>().intersections[i], tp.point);
            *out++ = tp;
        }

        return out;
    }
};

}} // namespace detail::get_intersection_points
#endif // DOXYGEN_NO_DETAIL




template
<
    typename Geometry1,
    typename Geometry2,
    typename Turns
>
inline void get_intersection_points(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            Turns& turns)
{
    concept::check_concepts_and_equal_dimensions<Geometry1 const, Geometry2 const>();

    typedef detail::get_intersection_points::get_turn_without_info
                        <
                            typename point_type<Geometry1>::type,
                            typename point_type<Geometry2>::type,
                            typename boost::range_value<Turns>::type
                        > TurnPolicy;

    typedef typename strategy_intersection
        <
            typename cs_tag<Geometry1>::type,
            Geometry1,
            Geometry2,
            typename boost::range_value<Turns>::type
        >::segment_intersection_strategy_type segment_intersection_strategy_type;

    detail::get_turns::no_interrupt_policy interrupt_policy;

    boost::mpl::if_c
        <
            reverse_dispatch<Geometry1, Geometry2>::type::value,
            dispatch::get_turns_reversed
            <
                typename tag<Geometry1>::type,
                typename tag<Geometry2>::type,
                Geometry1, Geometry2,
                false, false,
                Turns, TurnPolicy,
                //segment_intersection_strategy_type,
                detail::get_turns::no_interrupt_policy
            >,
            dispatch::get_turns
            <
                typename tag<Geometry1>::type,
                typename tag<Geometry2>::type,
                Geometry1, Geometry2,
                false, false,
                Turns, TurnPolicy,
                //segment_intersection_strategy_type,
                detail::get_turns::no_interrupt_policy
            >
        >::type::apply(
            0, geometry1,
            1, geometry2,
            turns, interrupt_policy);
}




}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_INTERSECTION_POINTS_HPP
