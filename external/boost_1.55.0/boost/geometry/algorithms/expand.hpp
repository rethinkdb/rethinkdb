// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_EXPAND_HPP
#define BOOST_GEOMETRY_ALGORITHMS_EXPAND_HPP


#include <cstddef>

#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/algorithms/not_implemented.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/util/select_coordinate_type.hpp>

#include <boost/geometry/strategies/compare.hpp>
#include <boost/geometry/policies/compare.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace expand
{


template
<
    typename StrategyLess, typename StrategyGreater,
    std::size_t Dimension, std::size_t DimensionCount
>
struct point_loop
{
    template <typename Box, typename Point>
    static inline void apply(Box& box, Point const& source)
    {
        typedef typename strategy::compare::detail::select_strategy
            <
                StrategyLess, 1, Point, Dimension
            >::type less_type;

        typedef typename strategy::compare::detail::select_strategy
            <
                StrategyGreater, -1, Point, Dimension
            >::type greater_type;

        typedef typename select_coordinate_type<Point, Box>::type coordinate_type;

        less_type less;
        greater_type greater;

        coordinate_type const coord = get<Dimension>(source);

        if (less(coord, get<min_corner, Dimension>(box)))
        {
            set<min_corner, Dimension>(box, coord);
        }

        if (greater(coord, get<max_corner, Dimension>(box)))
        {
            set<max_corner, Dimension>(box, coord);
        }

        point_loop
            <
                StrategyLess, StrategyGreater,
                Dimension + 1, DimensionCount
            >::apply(box, source);
    }
};


template
<
    typename StrategyLess, typename StrategyGreater,
    std::size_t DimensionCount
>
struct point_loop
    <
        StrategyLess, StrategyGreater,
        DimensionCount, DimensionCount
    >
{
    template <typename Box, typename Point>
    static inline void apply(Box&, Point const&) {}
};


template
<
    typename StrategyLess, typename StrategyGreater,
    std::size_t Index,
    std::size_t Dimension, std::size_t DimensionCount
>
struct indexed_loop
{
    template <typename Box, typename Geometry>
    static inline void apply(Box& box, Geometry const& source)
    {
        typedef typename strategy::compare::detail::select_strategy
            <
                StrategyLess, 1, Box, Dimension
            >::type less_type;

        typedef typename strategy::compare::detail::select_strategy
            <
                StrategyGreater, -1, Box, Dimension
            >::type greater_type;

        typedef typename select_coordinate_type
                <
                    Box,
                    Geometry
                >::type coordinate_type;

        less_type less;
        greater_type greater;

        coordinate_type const coord = get<Index, Dimension>(source);

        if (less(coord, get<min_corner, Dimension>(box)))
        {
            set<min_corner, Dimension>(box, coord);
        }

        if (greater(coord, get<max_corner, Dimension>(box)))
        {
            set<max_corner, Dimension>(box, coord);
        }

        indexed_loop
            <
                StrategyLess, StrategyGreater,
                Index, Dimension + 1, DimensionCount
            >::apply(box, source);
    }
};


template
<
    typename StrategyLess, typename StrategyGreater,
    std::size_t Index, std::size_t DimensionCount
>
struct indexed_loop
    <
        StrategyLess, StrategyGreater,
        Index, DimensionCount, DimensionCount
    >
{
    template <typename Box, typename Geometry>
    static inline void apply(Box&, Geometry const&) {}
};



// Changes a box such that the other box is also contained by the box
template
<
    typename StrategyLess, typename StrategyGreater
>
struct expand_indexed
{
    template <typename Box, typename Geometry>
    static inline void apply(Box& box, Geometry const& geometry)
    {
        indexed_loop
            <
                StrategyLess, StrategyGreater,
                0, 0, dimension<Geometry>::type::value
            >::apply(box, geometry);

        indexed_loop
            <
                StrategyLess, StrategyGreater,
                1, 0, dimension<Geometry>::type::value
            >::apply(box, geometry);
    }
};

}} // namespace detail::expand
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename GeometryOut, typename Geometry,
    typename StrategyLess = strategy::compare::default_strategy,
    typename StrategyGreater = strategy::compare::default_strategy,
    typename TagOut = typename tag<GeometryOut>::type,
    typename Tag = typename tag<Geometry>::type
>
struct expand: not_implemented<TagOut, Tag>
{};


// Box + point -> new box containing also point
template
<
    typename BoxOut, typename Point,
    typename StrategyLess, typename StrategyGreater
>
struct expand<BoxOut, Point, StrategyLess, StrategyGreater, box_tag, point_tag>
    : detail::expand::point_loop
        <
            StrategyLess, StrategyGreater,
            0, dimension<Point>::type::value
        >
{};


// Box + box -> new box containing two input boxes
template
<
    typename BoxOut, typename BoxIn,
    typename StrategyLess, typename StrategyGreater
>
struct expand<BoxOut, BoxIn, StrategyLess, StrategyGreater, box_tag, box_tag>
    : detail::expand::expand_indexed<StrategyLess, StrategyGreater>
{};

template
<
    typename Box, typename Segment,
    typename StrategyLess, typename StrategyGreater
>
struct expand<Box, Segment, StrategyLess, StrategyGreater, box_tag, segment_tag>
    : detail::expand::expand_indexed<StrategyLess, StrategyGreater>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/***
*!
\brief Expands a box using the extend (envelope) of another geometry (box, point)
\ingroup expand
\tparam Box type of the box
\tparam Geometry of second geometry, to be expanded with the box
\param box box to expand another geometry with, might be changed
\param geometry other geometry
\param strategy_less
\param strategy_greater
\note Strategy is currently ignored
 *
template
<
    typename Box, typename Geometry,
    typename StrategyLess, typename StrategyGreater
>
inline void expand(Box& box, Geometry const& geometry,
            StrategyLess const& strategy_less,
            StrategyGreater const& strategy_greater)
{
    concept::check_concepts_and_equal_dimensions<Box, Geometry const>();

    dispatch::expand<Box, Geometry>::apply(box, geometry);
}
***/


/*!
\brief Expands a box using the bounding box (envelope) of another geometry (box, point)
\ingroup expand
\tparam Box type of the box
\tparam Geometry \tparam_geometry
\param box box to be expanded using another geometry, mutable
\param geometry \param_geometry geometry which envelope (bounding box) will be added to the box

\qbk{[include reference/algorithms/expand.qbk]}
 */
template <typename Box, typename Geometry>
inline void expand(Box& box, Geometry const& geometry)
{
    concept::check_concepts_and_equal_dimensions<Box, Geometry const>();

    dispatch::expand<Box, Geometry>::apply(box, geometry);
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_EXPAND_HPP
