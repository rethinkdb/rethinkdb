// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DISTANCE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DISTANCE_HPP


#include <boost/concept_check.hpp>
#include <boost/mpl/if.hpp>
#include <boost/range.hpp>
#include <boost/typeof/typeof.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/reverse_dispatch.hpp>
#include <boost/geometry/core/tag_cast.hpp>

#include <boost/geometry/algorithms/not_implemented.hpp>
#include <boost/geometry/algorithms/detail/throw_on_empty_input.hpp>

#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/strategies/default_distance_result.hpp>
#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/within.hpp>

#include <boost/geometry/views/closeable_view.hpp>
#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance
{

// To avoid spurious namespaces here:
using strategy::distance::services::return_type;

template <typename P1, typename P2, typename Strategy>
struct point_to_point
{
    static inline typename return_type<Strategy, P1, P2>::type
    apply(P1 const& p1, P2 const& p2, Strategy const& strategy)
    {
        boost::ignore_unused_variable_warning(strategy);
        return strategy.apply(p1, p2);
    }
};


template<typename Point, typename Segment, typename Strategy>
struct point_to_segment
{
    static inline typename return_type<Strategy, Point, typename point_type<Segment>::type>::type
    apply(Point const& point, Segment const& segment, Strategy const& )
    {
        typename strategy::distance::services::default_strategy
            <
                segment_tag,
                Point,
                typename point_type<Segment>::type,
                typename cs_tag<Point>::type,
                typename cs_tag<typename point_type<Segment>::type>::type,
                Strategy
            >::type segment_strategy;

        typename point_type<Segment>::type p[2];
        geometry::detail::assign_point_from_index<0>(segment, p[0]);
        geometry::detail::assign_point_from_index<1>(segment, p[1]);
        return segment_strategy.apply(point, p[0], p[1]);
    }
};


template
<
    typename Point,
    typename Range,
    closure_selector Closure,
    typename PPStrategy,
    typename PSStrategy
>
struct point_to_range
{
    typedef typename return_type<PSStrategy, Point, typename point_type<Range>::type>::type return_type;

    static inline return_type apply(Point const& point, Range const& range,
            PPStrategy const& pp_strategy, PSStrategy const& ps_strategy)
    {
        return_type const zero = return_type(0);

        if (boost::size(range) == 0)
        {
            return zero;
        }

        typedef typename closeable_view<Range const, Closure>::type view_type;

        view_type view(range);

        // line of one point: return point distance
        typedef typename boost::range_iterator<view_type const>::type iterator_type;
        iterator_type it = boost::begin(view);
        iterator_type prev = it++;
        if (it == boost::end(view))
        {
            return pp_strategy.apply(point, *boost::begin(view));
        }

        // Create comparable (more efficient) strategy
        typedef typename strategy::distance::services::comparable_type<PSStrategy>::type eps_strategy_type;
        eps_strategy_type eps_strategy = strategy::distance::services::get_comparable<PSStrategy>::apply(ps_strategy);

        // start with first segment distance
        return_type d = eps_strategy.apply(point, *prev, *it);
        return_type rd = ps_strategy.apply(point, *prev, *it);

        // check if other segments are closer
        for (++prev, ++it; it != boost::end(view); ++prev, ++it)
        {
            return_type const ds = eps_strategy.apply(point, *prev, *it);
            if (geometry::math::equals(ds, zero))
            {
                return ds;
            }
            else if (ds < d)
            {
                d = ds;
                rd = ps_strategy.apply(point, *prev, *it);
            }
        }

        return rd;
    }
};


template
<
    typename Point,
    typename Ring,
    closure_selector Closure,
    typename PPStrategy,
    typename PSStrategy
>
struct point_to_ring
{
    typedef std::pair
        <
            typename return_type<PPStrategy, Point, typename point_type<Ring>::type>::type, bool
        > distance_containment;

    static inline distance_containment apply(Point const& point,
                Ring const& ring,
                PPStrategy const& pp_strategy, PSStrategy const& ps_strategy)
    {
        return distance_containment
            (
                point_to_range
                    <
                        Point,
                        Ring,
                        Closure,
                        PPStrategy,
                        PSStrategy
                    >::apply(point, ring, pp_strategy, ps_strategy),
                geometry::within(point, ring)
            );
    }
};



template
<
    typename Point,
    typename Polygon,
    closure_selector Closure,
    typename PPStrategy,
    typename PSStrategy
>
struct point_to_polygon
{
    typedef typename return_type<PPStrategy, Point, typename point_type<Polygon>::type>::type return_type;
    typedef std::pair<return_type, bool> distance_containment;

    static inline distance_containment apply(Point const& point,
                Polygon const& polygon,
                PPStrategy const& pp_strategy, PSStrategy const& ps_strategy)
    {
        // Check distance to all rings
        typedef point_to_ring
            <
                Point,
                typename ring_type<Polygon>::type,
                Closure,
                PPStrategy,
                PSStrategy
            > per_ring;

        distance_containment dc = per_ring::apply(point,
                        exterior_ring(polygon), pp_strategy, ps_strategy);

        typename interior_return_type<Polygon const>::type rings
                    = interior_rings(polygon);
        for (BOOST_AUTO_TPL(it, boost::begin(rings)); it != boost::end(rings); ++it)
        {
            distance_containment dcr = per_ring::apply(point,
                            *it, pp_strategy, ps_strategy);
            if (dcr.first < dc.first)
            {
                dc.first = dcr.first;
            }
            // If it was inside, and also inside inner ring,
            // turn off the inside-flag, it is outside the polygon
            if (dc.second && dcr.second)
            {
                dc.second = false;
            }
        }
        return dc;
    }
};


// Helper metafunction for default strategy retrieval
template <typename Geometry1, typename Geometry2>
struct default_strategy
    : strategy::distance::services::default_strategy
          <
              point_tag,
              typename point_type<Geometry1>::type,
              typename point_type<Geometry2>::type
          >
{};


}} // namespace detail::distance
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


using strategy::distance::services::return_type;


template
<
    typename Geometry1, typename Geometry2,
    typename Strategy = typename detail::distance::default_strategy<Geometry1, Geometry2>::type,
    typename Tag1 = typename tag_cast<typename tag<Geometry1>::type, multi_tag>::type,
    typename Tag2 = typename tag_cast<typename tag<Geometry2>::type, multi_tag>::type,
    typename StrategyTag = typename strategy::distance::services::tag<Strategy>::type,
    bool Reverse = reverse_dispatch<Geometry1, Geometry2>::type::value
>
struct distance: not_implemented<Tag1, Tag2>
{};


// If reversal is needed, perform it
template
<
    typename Geometry1, typename Geometry2, typename Strategy,
    typename Tag1, typename Tag2, typename StrategyTag
>
struct distance
<
    Geometry1, Geometry2, Strategy,
    Tag1, Tag2, StrategyTag,
    true
>
    : distance<Geometry2, Geometry1, Strategy, Tag2, Tag1, StrategyTag, false>
{
    typedef typename strategy::distance::services::return_type
                     <
                         Strategy,
                         typename point_type<Geometry2>::type,
                         typename point_type<Geometry1>::type
                     >::type return_type;

    static inline return_type apply(
        Geometry1 const& g1,
        Geometry2 const& g2,
        Strategy const& strategy)
    {
        return distance
            <
                Geometry2, Geometry1, Strategy,
                Tag2, Tag1, StrategyTag,
                false
            >::apply(g2, g1, strategy);
    }
};


// Point-point
template <typename P1, typename P2, typename Strategy>
struct distance
    <
        P1, P2, Strategy,
        point_tag, point_tag, strategy_tag_distance_point_point,
        false
    >
    : detail::distance::point_to_point<P1, P2, Strategy>
{};


// Point-line version 1, where point-point strategy is specified
template <typename Point, typename Linestring, typename Strategy>
struct distance
<
    Point, Linestring, Strategy,
    point_tag, linestring_tag, strategy_tag_distance_point_point,
    false
>
{

    static inline typename return_type<Strategy, Point, typename point_type<Linestring>::type>::type
    apply(Point const& point,
          Linestring const& linestring,
          Strategy const& strategy)
    {
        typedef typename strategy::distance::services::default_strategy
                    <
                        segment_tag,
                        Point,
                        typename point_type<Linestring>::type,
                        typename cs_tag<Point>::type,
                        typename cs_tag<typename point_type<Linestring>::type>::type,
                        Strategy
                    >::type ps_strategy_type;

        return detail::distance::point_to_range
            <
                Point, Linestring, closed, Strategy, ps_strategy_type
            >::apply(point, linestring, strategy, ps_strategy_type());
    }
};


// Point-line version 2, where point-segment strategy is specified
template <typename Point, typename Linestring, typename Strategy>
struct distance
<
    Point, Linestring, Strategy,
    point_tag, linestring_tag, strategy_tag_distance_point_segment,
    false
>
{
    static inline typename return_type<Strategy, Point, typename point_type<Linestring>::type>::type
    apply(Point const& point,
          Linestring const& linestring,
          Strategy const& strategy)
    {
        typedef typename Strategy::point_strategy_type pp_strategy_type;
        return detail::distance::point_to_range
            <
                Point, Linestring, closed, pp_strategy_type, Strategy
            >::apply(point, linestring, pp_strategy_type(), strategy);
    }
};

// Point-ring , where point-segment strategy is specified
template <typename Point, typename Ring, typename Strategy>
struct distance
<
    Point, Ring, Strategy,
    point_tag, ring_tag, strategy_tag_distance_point_point,
    false
>
{
    typedef typename return_type<Strategy, Point, typename point_type<Ring>::type>::type return_type;

    static inline return_type apply(Point const& point,
            Ring const& ring,
            Strategy const& strategy)
    {
        typedef typename strategy::distance::services::default_strategy
            <
                segment_tag,
                Point,
                typename point_type<Ring>::type
            >::type ps_strategy_type;

        std::pair<return_type, bool>
            dc = detail::distance::point_to_ring
            <
                Point, Ring,
                geometry::closure<Ring>::value,
                Strategy, ps_strategy_type
            >::apply(point, ring, strategy, ps_strategy_type());

        return dc.second ? return_type(0) : dc.first;
    }
};


// Point-polygon , where point-segment strategy is specified
template <typename Point, typename Polygon, typename Strategy>
struct distance
<
    Point, Polygon, Strategy,
    point_tag, polygon_tag, strategy_tag_distance_point_point,
    false
>
{
    typedef typename return_type<Strategy, Point, typename point_type<Polygon>::type>::type return_type;

    static inline return_type apply(Point const& point,
            Polygon const& polygon,
            Strategy const& strategy)
    {
        typedef typename strategy::distance::services::default_strategy
            <
                segment_tag,
                Point,
                typename point_type<Polygon>::type
            >::type ps_strategy_type;

        std::pair<return_type, bool>
            dc = detail::distance::point_to_polygon
            <
                Point, Polygon,
                geometry::closure<Polygon>::value,
                Strategy, ps_strategy_type
            >::apply(point, polygon, strategy, ps_strategy_type());

        return dc.second ? return_type(0) : dc.first;
    }
};



// Point-segment version 1, with point-point strategy
template <typename Point, typename Segment, typename Strategy>
struct distance
<
    Point, Segment, Strategy,
    point_tag, segment_tag, strategy_tag_distance_point_point,
    false
> : detail::distance::point_to_segment<Point, Segment, Strategy>
{};

// Point-segment version 2, with point-segment strategy
template <typename Point, typename Segment, typename Strategy>
struct distance
<
    Point, Segment, Strategy,
    point_tag, segment_tag, strategy_tag_distance_point_segment,
    false
>
{
    static inline typename return_type<Strategy, Point, typename point_type<Segment>::type>::type
    apply(Point const& point,
          Segment const& segment,
          Strategy const& strategy)
    {
        
        typename point_type<Segment>::type p[2];
        geometry::detail::assign_point_from_index<0>(segment, p[0]);
        geometry::detail::assign_point_from_index<1>(segment, p[1]);
        return strategy.apply(point, p[0], p[1]);
    }
};



} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

/*!
\brief \brief_calc2{distance} \brief_strategy
\ingroup distance
\details
\details \details_calc{area}. \brief_strategy. \details_strategy_reasons

\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam Strategy \tparam_strategy{Distance}
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param strategy \param_strategy{distance}
\return \return_calc{distance}
\note The strategy can be a point-point strategy. In case of distance point-line/point-polygon
    it may also be a point-segment strategy.

\qbk{distinguish,with strategy}

\qbk{
[heading Available Strategies]
\* [link geometry.reference.strategies.strategy_distance_pythagoras Pythagoras (cartesian)]
\* [link geometry.reference.strategies.strategy_distance_haversine Haversine (spherical)]
\* [link geometry.reference.strategies.strategy_distance_cross_track Cross track (spherical\, point-to-segment)]
\* [link geometry.reference.strategies.strategy_distance_projected_point Projected point (cartesian\, point-to-segment)]
\* more (currently extensions): Vincenty\, Andoyer (geographic)
}
 */

/*
Note, in case of a Compilation Error:
if you get:
 - "Failed to specialize function template ..."
 - "error: no matching function for call to ..."
for distance, it is probably so that there is no specialization
for return_type<...> for your strategy.
*/
template <typename Geometry1, typename Geometry2, typename Strategy>
inline typename strategy::distance::services::return_type
                <
                    Strategy,
                    typename point_type<Geometry1>::type,
                    typename point_type<Geometry2>::type
                >::type
distance(Geometry1 const& geometry1,
         Geometry2 const& geometry2,
         Strategy const& strategy)
{
    concept::check<Geometry1 const>();
    concept::check<Geometry2 const>();
    
    detail::throw_on_empty_input(geometry1);
    detail::throw_on_empty_input(geometry2);

    return dispatch::distance
               <
                   Geometry1,
                   Geometry2,
                   Strategy
               >::apply(geometry1, geometry2, strategy);
}


/*!
\brief \brief_calc2{distance}
\ingroup distance
\details The default strategy is used, corresponding to the coordinate system of the geometries
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\return \return_calc{distance}

\qbk{[include reference/algorithms/distance.qbk]}
 */
template <typename Geometry1, typename Geometry2>
inline typename default_distance_result<Geometry1, Geometry2>::type distance(
                Geometry1 const& geometry1, Geometry2 const& geometry2)
{
    concept::check<Geometry1 const>();
    concept::check<Geometry2 const>();

    return distance(geometry1, geometry2,
                    typename detail::distance::default_strategy<Geometry1, Geometry2>::type());
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DISTANCE_HPP
