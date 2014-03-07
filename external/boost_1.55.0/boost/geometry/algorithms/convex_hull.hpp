// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_CONVEX_HULL_HPP
#define BOOST_GEOMETRY_ALGORITHMS_CONVEX_HULL_HPP

#include <boost/array.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/exterior_ring.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/strategies/convex_hull.hpp>
#include <boost/geometry/strategies/concepts/convex_hull_concept.hpp>

#include <boost/geometry/views/detail/range_type.hpp>

#include <boost/geometry/algorithms/num_points.hpp>
#include <boost/geometry/algorithms/detail/as_range.hpp>
#include <boost/geometry/algorithms/detail/assign_box_corners.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace convex_hull
{

template <order_selector Order>
struct hull_insert
{

    // Member template function (to avoid inconvenient declaration
    // of output-iterator-type, from hull_to_geometry)
    template <typename Geometry, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Geometry const& geometry,
            OutputIterator out, Strategy const& strategy)
    {
        typename Strategy::state_type state;

        strategy.apply(geometry, state);
        strategy.result(state, out, Order == clockwise);
        return out;
    }
};

struct hull_to_geometry
{
    template <typename Geometry, typename OutputGeometry, typename Strategy>
    static inline void apply(Geometry const& geometry, OutputGeometry& out,
            Strategy const& strategy)
    {
        hull_insert
            <
                geometry::point_order<OutputGeometry>::value
            >::apply(geometry,
                std::back_inserter(
                    // Handle linestring, ring and polygon the same:
                    detail::as_range
                        <
                            typename range_type<OutputGeometry>::type
                        >(out)), strategy);
    }
};


// Helper metafunction for default strategy retrieval
template <typename Geometry>
struct default_strategy
    : strategy_convex_hull
          <
              Geometry,
              typename point_type<Geometry>::type
          >
{};


}} // namespace detail::convex_hull
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Geometry,
    typename Tag = typename tag<Geometry>::type
>
struct convex_hull
    : detail::convex_hull::hull_to_geometry
{};

template <typename Box>
struct convex_hull<Box, box_tag>
{
    template <typename OutputGeometry, typename Strategy>
    static inline void apply(Box const& box, OutputGeometry& out,
            Strategy const& )
    {
        static bool const Close
            = geometry::closure<OutputGeometry>::value == closed;
        static bool const Reverse
            = geometry::point_order<OutputGeometry>::value == counterclockwise;

        // A hull for boxes is trivial. Any strategy is (currently) skipped.
        boost::array<typename point_type<Box>::type, 4> range;
        geometry::detail::assign_box_corners_oriented<Reverse>(box, range);
        geometry::append(out, range);
        if (Close)
        {
            geometry::append(out, *boost::begin(range));
        }
    }
};



template <order_selector Order>
struct convex_hull_insert
    : detail::convex_hull::hull_insert<Order>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


template<typename Geometry, typename OutputGeometry, typename Strategy>
inline void convex_hull(Geometry const& geometry,
            OutputGeometry& out, Strategy const& strategy)
{
    concept::check_concepts_and_equal_dimensions
        <
            const Geometry,
            OutputGeometry
        >();

    BOOST_CONCEPT_ASSERT( (geometry::concept::ConvexHullStrategy<Strategy>) );

    if (geometry::num_points(geometry) == 0)
    {
        // Leave output empty
        return;
    }

    dispatch::convex_hull<Geometry>::apply(geometry, out, strategy);
}


/*!
\brief \brief_calc{convex hull}
\ingroup convex_hull
\details \details_calc{convex_hull,convex hull}.
\tparam Geometry the input geometry type
\tparam OutputGeometry the output geometry type
\param geometry \param_geometry,  input geometry
\param hull \param_geometry \param_set{convex hull}

\qbk{[include reference/algorithms/convex_hull.qbk]}
 */
template<typename Geometry, typename OutputGeometry>
inline void convex_hull(Geometry const& geometry,
            OutputGeometry& hull)
{
    concept::check_concepts_and_equal_dimensions
        <
            const Geometry,
            OutputGeometry
        >();

    typedef typename detail::convex_hull::default_strategy<Geometry>::type strategy_type;

    convex_hull(geometry, hull, strategy_type());
}

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace convex_hull
{


template<typename Geometry, typename OutputIterator, typename Strategy>
inline OutputIterator convex_hull_insert(Geometry const& geometry,
            OutputIterator out, Strategy const& strategy)
{
    // Concept: output point type = point type of input geometry
    concept::check<Geometry const>();
    concept::check<typename point_type<Geometry>::type>();

    BOOST_CONCEPT_ASSERT( (geometry::concept::ConvexHullStrategy<Strategy>) );

    return dispatch::convex_hull_insert
        <
            geometry::point_order<Geometry>::value
        >::apply(geometry, out, strategy);
}


/*!
\brief Calculate the convex hull of a geometry, output-iterator version
\ingroup convex_hull
\tparam Geometry the input geometry type
\tparam OutputIterator: an output-iterator
\param geometry the geometry to calculate convex hull from
\param out an output iterator outputing points of the convex hull
\note This overloaded version outputs to an output iterator.
In this case, nothing is known about its point-type or
    about its clockwise order. Therefore, the input point-type
    and order are copied

 */
template<typename Geometry, typename OutputIterator>
inline OutputIterator convex_hull_insert(Geometry const& geometry,
            OutputIterator out)
{
    // Concept: output point type = point type of input geometry
    concept::check<Geometry const>();
    concept::check<typename point_type<Geometry>::type>();

    typedef typename detail::convex_hull::default_strategy<Geometry>::type strategy_type;

    return convex_hull_insert(geometry, out, strategy_type());
}


}} // namespace detail::convex_hull
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_CONVEX_HULL_HPP
