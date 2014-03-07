// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_WITHIN_HPP
#define BOOST_GEOMETRY_ALGORITHMS_WITHIN_HPP


#include <cstddef>

#include <boost/concept_check.hpp>
#include <boost/range.hpp>
#include <boost/typeof/typeof.hpp>

#include <boost/geometry/algorithms/make.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/strategies/within.hpp>
#include <boost/geometry/strategies/concepts/within_concept.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/order_as_direction.hpp>
#include <boost/geometry/views/closeable_view.hpp>
#include <boost/geometry/views/reversible_view.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace within
{


template
<
    typename Point,
    typename Ring,
    iterate_direction Direction,
    closure_selector Closure,
    typename Strategy
>
struct point_in_ring
{
    BOOST_CONCEPT_ASSERT( (geometry::concept::WithinStrategyPolygonal<Strategy>) );

    static inline int apply(Point const& point, Ring const& ring,
            Strategy const& strategy)
    {
        boost::ignore_unused_variable_warning(strategy);
        if (int(boost::size(ring))
                < core_detail::closure::minimum_ring_size<Closure>::value)
        {
            return -1;
        }

        typedef typename reversible_view<Ring const, Direction>::type rev_view_type;
        typedef typename closeable_view
            <
                rev_view_type const, Closure
            >::type cl_view_type;
        typedef typename boost::range_iterator<cl_view_type const>::type iterator_type;

        rev_view_type rev_view(ring);
        cl_view_type view(rev_view);
        typename Strategy::state_type state;
        iterator_type it = boost::begin(view);
        iterator_type end = boost::end(view);

        bool stop = false;
        for (iterator_type previous = it++;
            it != end && ! stop;
            ++previous, ++it)
        {
            if (! strategy.apply(point, *previous, *it, state))
            {
                stop = true;
            }
        }

        return strategy.result(state);
    }
};


// Polygon: in exterior ring, and if so, not within interior ring(s)
template
<
    typename Point,
    typename Polygon,
    iterate_direction Direction,
    closure_selector Closure,
    typename Strategy
>
struct point_in_polygon
{
    BOOST_CONCEPT_ASSERT( (geometry::concept::WithinStrategyPolygonal<Strategy>) );

    static inline int apply(Point const& point, Polygon const& poly,
            Strategy const& strategy)
    {
        int const code = point_in_ring
            <
                Point,
                typename ring_type<Polygon>::type,
                Direction,
                Closure,
                Strategy
            >::apply(point, exterior_ring(poly), strategy);

        if (code == 1)
        {
            typename interior_return_type<Polygon const>::type rings
                        = interior_rings(poly);
            for (BOOST_AUTO_TPL(it, boost::begin(rings));
                it != boost::end(rings);
                ++it)
            {
                int const interior_code = point_in_ring
                    <
                        Point,
                        typename ring_type<Polygon>::type,
                        Direction,
                        Closure,
                        Strategy
                    >::apply(point, *it, strategy);

                if (interior_code != -1)
                {
                    // If 0, return 0 (touch)
                    // If 1 (inside hole) return -1 (outside polygon)
                    // If -1 (outside hole) check other holes if any
                    return -interior_code;
                }
            }
        }
        return code;
    }
};

}} // namespace detail::within
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Geometry1,
    typename Geometry2,
    typename Tag1 = typename tag<Geometry1>::type,
    typename Tag2 = typename tag<Geometry2>::type
>
struct within: not_implemented<Tag1, Tag2>
{};


template <typename Point, typename Box>
struct within<Point, Box, point_tag, box_tag>
{
    template <typename Strategy>
    static inline bool apply(Point const& point, Box const& box, Strategy const& strategy)
    {
        boost::ignore_unused_variable_warning(strategy);
        return strategy.apply(point, box);
    }
};

template <typename Box1, typename Box2>
struct within<Box1, Box2, box_tag, box_tag>
{
    template <typename Strategy>
    static inline bool apply(Box1 const& box1, Box2 const& box2, Strategy const& strategy)
    {
        assert_dimension_equal<Box1, Box2>();
        boost::ignore_unused_variable_warning(strategy);
        return strategy.apply(box1, box2);
    }
};



template <typename Point, typename Ring>
struct within<Point, Ring, point_tag, ring_tag>
{
    template <typename Strategy>
    static inline bool apply(Point const& point, Ring const& ring, Strategy const& strategy)
    {
        return detail::within::point_in_ring
            <
                Point,
                Ring,
                order_as_direction<geometry::point_order<Ring>::value>::value,
                geometry::closure<Ring>::value,
                Strategy
            >::apply(point, ring, strategy) == 1;
    }
};

template <typename Point, typename Polygon>
struct within<Point, Polygon, point_tag, polygon_tag>
{
    template <typename Strategy>
    static inline bool apply(Point const& point, Polygon const& polygon, Strategy const& strategy)
    {
        return detail::within::point_in_polygon
            <
                Point,
                Polygon,
                order_as_direction<geometry::point_order<Polygon>::value>::value,
                geometry::closure<Polygon>::value,
                Strategy
            >::apply(point, polygon, strategy) == 1;
    }
};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
\brief \brief_check12{is completely inside}
\ingroup within
\details \details_check12{within, is completely inside}.
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry which might be within the second geometry
\param geometry2 \param_geometry which might contain the first geometry
\return true if geometry1 is completely contained within geometry2,
    else false
\note The default strategy is used for within detection


\qbk{[include reference/algorithms/within.qbk]}

\qbk{
[heading Example]
[within]
[within_output]
}
 */
template<typename Geometry1, typename Geometry2>
inline bool within(Geometry1 const& geometry1, Geometry2 const& geometry2)
{
    concept::check<Geometry1 const>();
    concept::check<Geometry2 const>();
    assert_dimension_equal<Geometry1, Geometry2>();

    typedef typename point_type<Geometry1>::type point_type1;
    typedef typename point_type<Geometry2>::type point_type2;

    typedef typename strategy::within::services::default_strategy
        <
            typename tag<Geometry1>::type,
            typename tag<Geometry2>::type,
            typename tag<Geometry1>::type,
            typename tag_cast<typename tag<Geometry2>::type, areal_tag>::type,
            typename tag_cast
                <
                    typename cs_tag<point_type1>::type, spherical_tag
                >::type,
            typename tag_cast
                <
                    typename cs_tag<point_type2>::type, spherical_tag
                >::type,
            Geometry1,
            Geometry2
        >::type strategy_type;

    return dispatch::within
        <
            Geometry1,
            Geometry2
        >::apply(geometry1, geometry2, strategy_type());
}

/*!
\brief \brief_check12{is completely inside} \brief_strategy
\ingroup within
\details \details_check12{within, is completely inside}, \brief_strategy. \details_strategy_reasons
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry which might be within the second geometry
\param geometry2 \param_geometry which might contain the first geometry
\param strategy strategy to be used
\return true if geometry1 is completely contained within geometry2,
    else false

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/within.qbk]}
\qbk{
[heading Available Strategies]
\* [link geometry.reference.strategies.strategy_within_winding Winding (coordinate system agnostic)]
\* [link geometry.reference.strategies.strategy_within_franklin Franklin (cartesian)]
\* [link geometry.reference.strategies.strategy_within_crossings_multiply Crossings Multiply (cartesian)]

[heading Example]
[within_strategy]
[within_strategy_output]

}
*/
template<typename Geometry1, typename Geometry2, typename Strategy>
inline bool within(Geometry1 const& geometry1, Geometry2 const& geometry2,
        Strategy const& strategy)
{
    concept::within::check
        <
            typename tag<Geometry1>::type, 
            typename tag<Geometry2>::type, 
            typename tag_cast<typename tag<Geometry2>::type, areal_tag>::type,
            Strategy
        >();
    concept::check<Geometry1 const>();
    concept::check<Geometry2 const>();
    assert_dimension_equal<Geometry1, Geometry2>();

    return dispatch::within
        <
            Geometry1,
            Geometry2
        >::apply(geometry1, geometry2, strategy);
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_WITHIN_HPP
