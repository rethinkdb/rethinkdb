// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_TOUCHES_HPP
#define BOOST_GEOMETRY_ALGORITHMS_TOUCHES_HPP


#include <deque>

#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/algorithms/detail/overlay/self_turn_points.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>
#include <boost/geometry/algorithms/disjoint.hpp>
#include <boost/geometry/algorithms/intersects.hpp>
#include <boost/geometry/algorithms/num_geometries.hpp>


namespace boost { namespace geometry
{

namespace detail { namespace touches 
{

template <typename Turn>
inline bool ok_for_touch(Turn const& turn)
{
    return turn.both(detail::overlay::operation_union)
        || turn.both(detail::overlay::operation_blocked)
        || turn.combination(detail::overlay::operation_union, detail::overlay::operation_blocked)
        ;
}

template <typename Turns>
inline bool has_only_turns(Turns const& turns)
{
    bool has_touch = false;
    typedef typename boost::range_iterator<Turns const>::type iterator_type;
    for (iterator_type it = boost::begin(turns); it != boost::end(turns); ++it)
    {
        if (it->has(detail::overlay::operation_intersection))
        {
            return false;
        }

        switch(it->method)
        {
            case detail::overlay::method_crosses: 
                return false;
            case detail::overlay::method_equal: 
                // Segment spatially equal means: at the right side
                // the polygon internally overlaps. So return false.
                return false;
            case detail::overlay::method_touch: 
            case detail::overlay::method_touch_interior: 
            case detail::overlay::method_collinear: 
                if (ok_for_touch(*it))
                {
                    has_touch = true;
                }
                else
                {
                    return false;
                }
                break;
            case detail::overlay::method_none :
            case detail::overlay::method_disjoint :
            case detail::overlay::method_error :
                break;
        }
    }
    return has_touch;
}

}}

/*!
\brief \brief_check{has at least one touching point (self-tangency)}
\note This function can be called for one geometry (self-tangency) and
    also for two geometries (touch)
\ingroup touches
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\return \return_check{is self-touching}

\qbk{distinguish,one geometry}
\qbk{[def __one_parameter__]}
\qbk{[include reference/algorithms/touches.qbk]}
*/
template <typename Geometry>
inline bool touches(Geometry const& geometry)
{
    concept::check<Geometry const>();

    typedef detail::overlay::turn_info
        <
            typename geometry::point_type<Geometry>::type
        > turn_info;

    typedef detail::overlay::get_turn_info
        <
            typename point_type<Geometry>::type,
            typename point_type<Geometry>::type,
            turn_info,
            detail::overlay::assign_null_policy
        > policy_type;

    std::deque<turn_info> turns;
    detail::self_get_turn_points::no_interrupt_policy policy;
    detail::self_get_turn_points::get_turns
            <
                Geometry,
                std::deque<turn_info>,
                policy_type,
                detail::self_get_turn_points::no_interrupt_policy
            >::apply(geometry, turns, policy);

    return detail::touches::has_only_turns(turns);
}


/*!
\brief \brief_check2{have at least one touching point (tangent - non overlapping)}
\ingroup touches
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\return \return_check2{touch each other}

\qbk{distinguish,two geometries}
\qbk{[include reference/algorithms/touches.qbk]}
 */
template <typename Geometry1, typename Geometry2>
inline bool touches(Geometry1 const& geometry1, Geometry2 const& geometry2)
{
    concept::check<Geometry1 const>();
    concept::check<Geometry2 const>();


    typedef detail::overlay::turn_info
        <
            typename geometry::point_type<Geometry1>::type
        > turn_info;

    typedef detail::overlay::get_turn_info
        <
            typename point_type<Geometry1>::type,
            typename point_type<Geometry2>::type,
            turn_info,
            detail::overlay::assign_null_policy
        > policy_type;

    std::deque<turn_info> turns;
    detail::get_turns::no_interrupt_policy policy;
    boost::geometry::get_turns
            <
                false, false,
                detail::overlay::assign_null_policy
            >(geometry1, geometry2, turns, policy);

    return detail::touches::has_only_turns(turns)
        && ! geometry::detail::disjoint::rings_containing(geometry1, geometry2)
        && ! geometry::detail::disjoint::rings_containing(geometry2, geometry1)
        ;
}



}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_TOUCHES_HPP
