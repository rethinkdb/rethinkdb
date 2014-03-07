// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_SELF_TURN_POINTS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_SELF_TURN_POINTS_HPP

#include <cstddef>

#include <boost/range.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/algorithms/detail/disjoint.hpp>
#include <boost/geometry/algorithms/detail/partition.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>

#include <boost/geometry/geometries/box.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace self_get_turn_points
{

struct no_interrupt_policy
{
    static bool const enabled = false;
    static bool const has_intersections = false;


    template <typename Range>
    static inline bool apply(Range const&)
    {
        return false;
    }
};




class self_ip_exception : public geometry::exception {};

template
<
    typename Geometry,
    typename Turns,
    typename TurnPolicy,
    typename InterruptPolicy
>
struct self_section_visitor
{
    Geometry const& m_geometry;
    Turns& m_turns;
    InterruptPolicy& m_interrupt_policy;

    inline self_section_visitor(Geometry const& g,
            Turns& turns, InterruptPolicy& ip)
        : m_geometry(g)
        , m_turns(turns)
        , m_interrupt_policy(ip)
    {}

    template <typename Section>
    inline bool apply(Section const& sec1, Section const& sec2)
    {
        if (! detail::disjoint::disjoint_box_box(sec1.bounding_box, sec2.bounding_box)
                && ! sec1.duplicate
                && ! sec2.duplicate)
        {
            detail::get_turns::get_turns_in_sections
                    <
                        Geometry, Geometry,
                        false, false,
                        Section, Section,
                        Turns, TurnPolicy,
                        InterruptPolicy
                    >::apply(
                            0, m_geometry, sec1,
                            0, m_geometry, sec2,
                            false,
                            m_turns, m_interrupt_policy);
        }
        if (m_interrupt_policy.has_intersections)
        {
            // TODO: we should give partition an interrupt policy.
            // Now we throw, and catch below, to stop the partition loop.
            throw self_ip_exception();
        }
        return true;
    }

};



template
<
    typename Geometry,
    typename Turns,
    typename TurnPolicy,
    typename InterruptPolicy
>
struct get_turns
{
    static inline bool apply(
            Geometry const& geometry,
            Turns& turns,
            InterruptPolicy& interrupt_policy)
    {
        typedef model::box
            <
                typename geometry::point_type<Geometry>::type
            > box_type;
        typedef typename geometry::sections
            <
                box_type, 1
            > sections_type;

        sections_type sec;
        geometry::sectionalize<false>(geometry, sec);

        self_section_visitor
            <
                Geometry,
                Turns, TurnPolicy, InterruptPolicy
            > visitor(geometry, turns, interrupt_policy);

        try
        {
            geometry::partition
                <
                    box_type, 
                    detail::get_turns::get_section_box, 
                    detail::get_turns::ovelaps_section_box
                >::apply(sec, visitor);
        }
        catch(self_ip_exception const& )
        {
            return false;
        }

        return true;
    }
};


}} // namespace detail::self_get_turn_points
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename GeometryTag,
    typename Geometry,
    typename Turns,
    typename TurnPolicy,
    typename InterruptPolicy
>
struct self_get_turn_points
{
};


template
<
    typename Ring,
    typename Turns,
    typename TurnPolicy,
    typename InterruptPolicy
>
struct self_get_turn_points
    <
        ring_tag, Ring,
        Turns,
        TurnPolicy,
        InterruptPolicy
    >
    : detail::self_get_turn_points::get_turns
        <
            Ring,
            Turns,
            TurnPolicy,
            InterruptPolicy
        >
{};


template
<
    typename Box,
    typename Turns,
    typename TurnPolicy,
    typename InterruptPolicy
>
struct self_get_turn_points
    <
        box_tag, Box,
        Turns,
        TurnPolicy,
        InterruptPolicy
    >
{
    static inline bool apply(
            Box const& ,
            Turns& ,
            InterruptPolicy& )
    {
        return true;
    }
};


template
<
    typename Polygon,
    typename Turns,
    typename TurnPolicy,
    typename InterruptPolicy
>
struct self_get_turn_points
    <
        polygon_tag, Polygon,
        Turns,
        TurnPolicy,
        InterruptPolicy
    >
    : detail::self_get_turn_points::get_turns
        <
            Polygon,
            Turns,
            TurnPolicy,
            InterruptPolicy
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
    \brief Calculate self intersections of a geometry
    \ingroup overlay
    \tparam Geometry geometry type
    \tparam Turns type of intersection container
                (e.g. vector of "intersection/turn point"'s)
    \param geometry geometry
    \param turns container which will contain intersection points
    \param interrupt_policy policy determining if process is stopped
        when intersection is found
 */
template
<
    typename AssignPolicy,
    typename Geometry,
    typename Turns,
    typename InterruptPolicy
>
inline void self_turns(Geometry const& geometry,
            Turns& turns, InterruptPolicy& interrupt_policy)
{
    concept::check<Geometry const>();

    typedef detail::overlay::get_turn_info
                        <
                            typename point_type<Geometry>::type,
                            typename point_type<Geometry>::type,
                            typename boost::range_value<Turns>::type,
                            detail::overlay::assign_null_policy
                        > TurnPolicy;

    dispatch::self_get_turn_points
            <
                typename tag<Geometry>::type,
                Geometry,
                Turns,
                TurnPolicy,
                InterruptPolicy
            >::apply(geometry, turns, interrupt_policy);
}



}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_SELF_TURN_POINTS_HPP
