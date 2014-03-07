// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_OVERLAY_COPY_SEGMENTS_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_OVERLAY_COPY_SEGMENTS_HPP


#include <boost/assert.hpp>
#include <boost/range.hpp>

#include <boost/geometry/algorithms/detail/overlay/copy_segments.hpp>

#include <boost/geometry/multi/core/ring_type.hpp>
#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace copy_segments
{


template
<
    typename MultiGeometry,
    typename SegmentIdentifier,
    typename RangeOut,
    typename Policy
>
struct copy_segments_multi
{
    static inline void apply(MultiGeometry const& multi_geometry,
            SegmentIdentifier const& seg_id, int to_index,
            RangeOut& current_output)
    {

        BOOST_ASSERT
            (
                seg_id.multi_index >= 0
                && seg_id.multi_index < int(boost::size(multi_geometry))
            );

        // Call the single-version
        Policy::apply(multi_geometry[seg_id.multi_index],
                    seg_id, to_index, current_output);
    }
};


}} // namespace detail::copy_segments
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename MultiPolygon,
    bool Reverse,
    typename SegmentIdentifier,
    typename RangeOut
>
struct copy_segments
    <
        multi_polygon_tag,
        MultiPolygon,
        Reverse,
        SegmentIdentifier,
        RangeOut
    >
    : detail::copy_segments::copy_segments_multi
        <
            MultiPolygon,
            SegmentIdentifier,
            RangeOut,
            detail::copy_segments::copy_segments_polygon
                <
                    typename boost::range_value<MultiPolygon>::type,
                    Reverse,
                    SegmentIdentifier,
                    RangeOut
                >
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_OVERLAY_COPY_SEGMENTS_HPP
