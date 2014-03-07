// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_COPY_SEGMENTS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_COPY_SEGMENTS_HPP


#include <boost/array.hpp>
#include <boost/mpl/assert.hpp>
#include <vector>

#include <boost/assert.hpp>
#include <boost/range.hpp>

#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/iterators/ever_circling_iterator.hpp>
#include <boost/geometry/views/closeable_view.hpp>
#include <boost/geometry/views/reversible_view.hpp>

#include <boost/geometry/algorithms/detail/overlay/append_no_dups_or_spikes.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace copy_segments
{


template
<
    typename Ring,
    bool Reverse,
    typename SegmentIdentifier,
    typename RangeOut
>
struct copy_segments_ring
{
    typedef typename closeable_view
        <
            Ring const,
            closure<Ring>::value
        >::type cview_type;

    typedef typename reversible_view
        <
            cview_type const,
            Reverse ? iterate_reverse : iterate_forward
        >::type rview_type;

    typedef typename boost::range_iterator<rview_type const>::type iterator;
    typedef geometry::ever_circling_iterator<iterator> ec_iterator;

    static inline void apply(Ring const& ring,
            SegmentIdentifier const& seg_id, int to_index,
            RangeOut& current_output)
    {
        cview_type cview(ring);
        rview_type view(cview);

        // The problem: sometimes we want to from "3" to "2"
        // -> end = "3" -> end == begin
        // This is not convenient with iterators.

        // So we use the ever-circling iterator and determine when to step out

        int const from_index = seg_id.segment_index + 1;

        // Sanity check
        BOOST_ASSERT(from_index < int(boost::size(view)));

        ec_iterator it(boost::begin(view), boost::end(view),
                    boost::begin(view) + from_index);

        // [2..4] -> 4 - 2 + 1 = 3 -> {2,3,4} -> OK
        // [4..2],size=6 -> 6 - 4 + 2 + 1 = 5 -> {4,5,0,1,2} -> OK
        // [1..1], travel the whole ring round
        typedef typename boost::range_difference<Ring>::type size_type;
        size_type const count = from_index <= to_index
            ? to_index - from_index + 1
            : int(boost::size(view)) - from_index + to_index + 1;

        for (size_type i = 0; i < count; ++i, ++it)
        {
            detail::overlay::append_no_dups_or_spikes(current_output, *it);
        }
    }
};

template
<
    typename LineString,
    bool Reverse,
    typename SegmentIdentifier,
    typename RangeOut
>
struct copy_segments_linestring
{

    typedef typename boost::range_iterator<LineString const>::type iterator;

    static inline void apply(LineString const& ls,
            SegmentIdentifier const& seg_id, int to_index,
            RangeOut& current_output)
    {
        int const from_index = seg_id.segment_index + 1;

        // Sanity check
        if (from_index > to_index || from_index < 0 || to_index >= int(boost::size(ls)))
        {
            return;
        }

        typedef typename boost::range_difference<LineString>::type size_type;
        size_type const count = to_index - from_index + 1;

        typename boost::range_iterator<LineString const>::type it = boost::begin(ls) + from_index;

        for (size_type i = 0; i < count; ++i, ++it)
        {
            detail::overlay::append_no_dups_or_spikes(current_output, *it);
        }
    }
};

template
<
    typename Polygon,
    bool Reverse,
    typename SegmentIdentifier,
    typename RangeOut
>
struct copy_segments_polygon
{
    static inline void apply(Polygon const& polygon,
            SegmentIdentifier const& seg_id, int to_index,
            RangeOut& current_output)
    {
        // Call ring-version with the right ring
        copy_segments_ring
            <
                typename geometry::ring_type<Polygon>::type,
                Reverse,
                SegmentIdentifier,
                RangeOut
            >::apply
                (
                    seg_id.ring_index < 0
                    ? geometry::exterior_ring(polygon)
                    : geometry::interior_rings(polygon)[seg_id.ring_index],
                    seg_id, to_index,
                    current_output
                );
    }
};


template
<
    typename Box,
    bool Reverse,
    typename SegmentIdentifier,
    typename RangeOut
>
struct copy_segments_box
{
    static inline void apply(Box const& box,
            SegmentIdentifier const& seg_id, int to_index,
            RangeOut& current_output)
    {
        int index = seg_id.segment_index + 1;
        BOOST_ASSERT(index < 5);

        int const count = index <= to_index
            ? to_index - index + 1
            : 5 - index + to_index + 1;

        // Create array of points, the fifth one closes it
        boost::array<typename point_type<Box>::type, 5> bp;
        assign_box_corners_oriented<Reverse>(box, bp);
        bp[4] = bp[0];

        // (possibly cyclic) copy to output
        //    (see comments in ring-version)
        for (int i = 0; i < count; i++, index++)
        {
            detail::overlay::append_no_dups_or_spikes(current_output, bp[index % 5]);

        }
    }
};


}} // namespace detail::copy_segments
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Tag,
    typename GeometryIn,
    bool Reverse,
    typename SegmentIdentifier,
    typename RangeOut
>
struct copy_segments
{
    BOOST_MPL_ASSERT_MSG
        (
            false, NOT_OR_NOT_YET_IMPLEMENTED_FOR_THIS_GEOMETRY_TYPE
            , (types<GeometryIn>)
        );
};


template
<
    typename Ring,
    bool Reverse,
    typename SegmentIdentifier,
    typename RangeOut
>
struct copy_segments<ring_tag, Ring, Reverse, SegmentIdentifier, RangeOut>
    : detail::copy_segments::copy_segments_ring
        <
            Ring, Reverse, SegmentIdentifier, RangeOut
        >
{};



template
<
    typename LineString,
    bool Reverse,
    typename SegmentIdentifier,
    typename RangeOut
>
struct copy_segments<linestring_tag, LineString, Reverse, SegmentIdentifier, RangeOut>
    : detail::copy_segments::copy_segments_linestring
        <
            LineString, Reverse, SegmentIdentifier, RangeOut
        >
{};

template
<
    typename Polygon,
    bool Reverse,
    typename SegmentIdentifier,
    typename RangeOut
>
struct copy_segments<polygon_tag, Polygon, Reverse, SegmentIdentifier, RangeOut>
    : detail::copy_segments::copy_segments_polygon
        <
            Polygon, Reverse, SegmentIdentifier, RangeOut
        >
{};


template
<
    typename Box,
    bool Reverse,
    typename SegmentIdentifier,
    typename RangeOut
>
struct copy_segments<box_tag, Box, Reverse, SegmentIdentifier, RangeOut>
    : detail::copy_segments::copy_segments_box
        <
            Box, Reverse, SegmentIdentifier, RangeOut
        >
{};



} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
    \brief Copy segments from a geometry, starting with the specified segment (seg_id)
        until the specified index (to_index)
    \ingroup overlay
 */
template
<
    bool Reverse,
    typename Geometry,
    typename SegmentIdentifier,
    typename RangeOut
>
inline void copy_segments(Geometry const& geometry,
            SegmentIdentifier const& seg_id, int to_index,
            RangeOut& range_out)
{
    concept::check<Geometry const>();

    dispatch::copy_segments
        <
            typename tag<Geometry>::type,
            Geometry,
            Reverse,
            SegmentIdentifier,
            RangeOut
        >::apply(geometry, seg_id, to_index, range_out);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_COPY_SEGMENTS_HPP
