// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_COPY_SEGMENT_POINT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_COPY_SEGMENT_POINT_HPP


#include <boost/array.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/range.hpp>

#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/views/closeable_view.hpp>
#include <boost/geometry/views/reversible_view.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace copy_segments
{


template <typename Range, bool Reverse, typename SegmentIdentifier, typename PointOut>
struct copy_segment_point_range
{
    typedef typename closeable_view
        <
            Range const,
            closure<Range>::value
        >::type cview_type;

    typedef typename reversible_view
        <
            cview_type const,
            Reverse ? iterate_reverse : iterate_forward
        >::type rview_type;

    static inline bool apply(Range const& range,
            SegmentIdentifier const& seg_id, bool second,
            PointOut& point)
    {
        int index = seg_id.segment_index;
        if (second)
        {
            index++;
            if (index >= int(boost::size(range)))
            {
                index = 0;
            }
        }

        // Exception?
        if (index >= int(boost::size(range)))
        {
            return false;
        }

        cview_type cview(range);
        rview_type view(cview);


        geometry::convert(*(boost::begin(view) + index), point);
        return true;
    }
};


template <typename Polygon, bool Reverse, typename SegmentIdentifier, typename PointOut>
struct copy_segment_point_polygon
{
    static inline bool apply(Polygon const& polygon,
            SegmentIdentifier const& seg_id, bool second,
            PointOut& point)
    {
        // Call ring-version with the right ring
        return copy_segment_point_range
            <
                typename geometry::ring_type<Polygon>::type,
                Reverse,
                SegmentIdentifier,
                PointOut
            >::apply
                (
                    seg_id.ring_index < 0
                    ? geometry::exterior_ring(polygon)
                    : geometry::interior_rings(polygon)[seg_id.ring_index],
                    seg_id, second,
                    point
                );
    }
};


template <typename Box, bool Reverse, typename SegmentIdentifier, typename PointOut>
struct copy_segment_point_box
{
    static inline bool apply(Box const& box,
            SegmentIdentifier const& seg_id, bool second,
            PointOut& point)
    {
        int index = seg_id.segment_index;
        if (second)
        {
            index++;
        }

        boost::array<typename point_type<Box>::type, 4> bp;
        assign_box_corners_oriented<Reverse>(box, bp);
        point = bp[index % 4];
        return true;
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
    typename PointOut
>
struct copy_segment_point
{
    BOOST_MPL_ASSERT_MSG
        (
            false, NOT_OR_NOT_YET_IMPLEMENTED_FOR_THIS_GEOMETRY_TYPE
            , (types<GeometryIn>)
        );
};


template <typename LineString, bool Reverse, typename SegmentIdentifier, typename PointOut>
struct copy_segment_point<linestring_tag, LineString, Reverse, SegmentIdentifier, PointOut>
    : detail::copy_segments::copy_segment_point_range
        <
            LineString, Reverse, SegmentIdentifier, PointOut
        >
{};


template <typename Ring, bool Reverse, typename SegmentIdentifier, typename PointOut>
struct copy_segment_point<ring_tag, Ring, Reverse, SegmentIdentifier, PointOut>
    : detail::copy_segments::copy_segment_point_range
        <
            Ring, Reverse, SegmentIdentifier, PointOut
        >
{};

template <typename Polygon, bool Reverse, typename SegmentIdentifier, typename PointOut>
struct copy_segment_point<polygon_tag, Polygon, Reverse, SegmentIdentifier, PointOut>
    : detail::copy_segments::copy_segment_point_polygon
        <
            Polygon, Reverse, SegmentIdentifier, PointOut
        >
{};


template <typename Box, bool Reverse, typename SegmentIdentifier, typename PointOut>
struct copy_segment_point<box_tag, Box, Reverse, SegmentIdentifier, PointOut>
    : detail::copy_segments::copy_segment_point_box
        <
            Box, Reverse, SegmentIdentifier, PointOut
        >
{};



} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH





/*!
    \brief Helper function, copies a point from a segment
    \ingroup overlay
 */
template<bool Reverse, typename Geometry, typename SegmentIdentifier, typename PointOut>
inline bool copy_segment_point(Geometry const& geometry,
            SegmentIdentifier const& seg_id, bool second,
            PointOut& point_out)
{
    concept::check<Geometry const>();

    return dispatch::copy_segment_point
        <
            typename tag<Geometry>::type,
            Geometry,
            Reverse,
            SegmentIdentifier,
            PointOut
        >::apply(geometry, seg_id, second, point_out);
}


/*!
    \brief Helper function, to avoid the same construct several times,
        copies a point, based on a source-index and two geometries
    \ingroup overlay
 */
template
<
    bool Reverse1, bool Reverse2,
    typename Geometry1, typename Geometry2,
    typename SegmentIdentifier,
    typename PointOut
>
inline bool copy_segment_point(Geometry1 const& geometry1, Geometry2 const& geometry2,
            SegmentIdentifier const& seg_id, bool second,
            PointOut& point_out)
{
    concept::check<Geometry1 const>();
    concept::check<Geometry2 const>();

    if (seg_id.source_index == 0)
    {
        return dispatch::copy_segment_point
            <
                typename tag<Geometry1>::type,
                Geometry1,
                Reverse1,
                SegmentIdentifier,
                PointOut
            >::apply(geometry1, seg_id, second, point_out);
    }
    else if (seg_id.source_index == 1)
    {
        return dispatch::copy_segment_point
            <
                typename tag<Geometry2>::type,
                Geometry2,
                Reverse2,
                SegmentIdentifier,
                PointOut
            >::apply(geometry2, seg_id, second, point_out);
    }
    // Exception?
    return false;
}


/*!
    \brief Helper function, to avoid the same construct several times,
        copies a point, based on a source-index and two geometries
    \ingroup overlay
 */
template
<
    bool Reverse1, bool Reverse2,
    typename Geometry1, typename Geometry2,
    typename SegmentIdentifier,
    typename PointOut
>
inline bool copy_segment_points(Geometry1 const& geometry1, Geometry2 const& geometry2,
            SegmentIdentifier const& seg_id,
            PointOut& point1, PointOut& point2)
{
    concept::check<Geometry1 const>();
    concept::check<Geometry2 const>();

    return copy_segment_point<Reverse1, Reverse2>(geometry1, geometry2, seg_id, false, point1)
        && copy_segment_point<Reverse1, Reverse2>(geometry1, geometry2, seg_id, true,  point2);
}




}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_COPY_SEGMENT_POINT_HPP
