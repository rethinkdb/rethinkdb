// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_INTERSECTS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_INTERSECTS_HPP


#include <deque>

#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/algorithms/detail/overlay/self_turn_points.hpp>
#include <boost/geometry/algorithms/disjoint.hpp>


namespace boost { namespace geometry
{

/*!
\brief \brief_check{has at least one intersection (crossing or self-tangency)}
\note This function can be called for one geometry (self-intersection) and
    also for two geometries (intersection)
\ingroup intersects
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\return \return_check{is self-intersecting}

\qbk{distinguish,one geometry}
\qbk{[def __one_parameter__]}
\qbk{[include reference/algorithms/intersects.qbk]}
*/
template <typename Geometry>
inline bool intersects(Geometry const& geometry)
{
    concept::check<Geometry const>();


    typedef detail::overlay::turn_info
        <
            typename geometry::point_type<Geometry>::type
        > turn_info;
    std::deque<turn_info> turns;

    typedef typename strategy_intersection
        <
            typename cs_tag<Geometry>::type,
            Geometry,
            Geometry,
            typename geometry::point_type<Geometry>::type
        >::segment_intersection_strategy_type segment_intersection_strategy_type;

    typedef detail::overlay::get_turn_info
        <
            typename point_type<Geometry>::type,
            typename point_type<Geometry>::type,
            turn_info,
            detail::overlay::assign_null_policy
        > TurnPolicy;

    detail::disjoint::disjoint_interrupt_policy policy;
    detail::self_get_turn_points::get_turns
            <
                Geometry,
                std::deque<turn_info>,
                TurnPolicy,
                detail::disjoint::disjoint_interrupt_policy
            >::apply(geometry, turns, policy);
    return policy.has_intersections;
}


/*!
\brief \brief_check2{have at least one intersection}
\ingroup intersects
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\return \return_check2{intersect each other}

\qbk{distinguish,two geometries}
\qbk{[include reference/algorithms/intersects.qbk]}
 */
template <typename Geometry1, typename Geometry2>
inline bool intersects(Geometry1 const& geometry1, Geometry2 const& geometry2)
{
    concept::check<Geometry1 const>();
    concept::check<Geometry2 const>();

    return ! geometry::disjoint(geometry1, geometry2);
}



}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_INTERSECTS_HPP
