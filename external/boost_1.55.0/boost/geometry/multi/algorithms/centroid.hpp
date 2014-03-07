// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_CENTROID_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_CENTROID_HPP


#include <boost/range.hpp>

#include <boost/geometry/algorithms/centroid.hpp>
#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/core/point_type.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>
#include <boost/geometry/multi/algorithms/num_points.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace centroid
{


/*!
    \brief Building block of a multi-point, to be used as Policy in the
        more generec centroid_multi
*/
struct centroid_multi_point_state
{
    template <typename Point, typename Strategy>
    static inline void apply(Point const& point,
            Strategy const& strategy, typename Strategy::state_type& state)
    {
        strategy.apply(point, state);
    }
};



/*!
    \brief Generic implementation which calls a policy to calculate the
        centroid of the total of its single-geometries
    \details The Policy is, in general, the single-version, with state. So
        detail::centroid::centroid_polygon_state is used as a policy for this
        detail::centroid::centroid_multi

*/
template <typename Policy>
struct centroid_multi
{
    template <typename Multi, typename Point, typename Strategy>
    static inline void apply(Multi const& multi, Point& centroid,
            Strategy const& strategy)
    {
#if ! defined(BOOST_GEOMETRY_CENTROID_NO_THROW)
        // If there is nothing in any of the ranges, it is not possible
        // to calculate the centroid
        if (geometry::num_points(multi) == 0)
        {
            throw centroid_exception();
        }
#endif

        typename Strategy::state_type state;

        for (typename boost::range_iterator<Multi const>::type
                it = boost::begin(multi);
            it != boost::end(multi);
            ++it)
        {
            Policy::apply(*it, strategy, state);
        }
        Strategy::result(state, centroid);
    }
};



}} // namespace detail::centroid
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename MultiLinestring>
struct centroid<MultiLinestring, multi_linestring_tag>
    : detail::centroid::centroid_multi
        <
            detail::centroid::centroid_range_state<closed>
        >
{};

template <typename MultiPolygon>
struct centroid<MultiPolygon, multi_polygon_tag>
    : detail::centroid::centroid_multi
        <
            detail::centroid::centroid_polygon_state
        >
{};


template <typename MultiPoint>
struct centroid<MultiPoint, multi_point_tag>
    : detail::centroid::centroid_multi
        <
            detail::centroid::centroid_multi_point_state
        >
{};


} // namespace dispatch
#endif


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_CENTROID_HPP

