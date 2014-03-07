// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_SECTIONS_SECTIONALIZE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_SECTIONS_SECTIONALIZE_HPP

#include <cstddef>
#include <vector>

#include <boost/mpl/assert.hpp>
#include <boost/range.hpp>
#include <boost/typeof/typeof.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/expand.hpp>

#include <boost/geometry/algorithms/detail/ring_identifier.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/point_order.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/views/closeable_view.hpp>
#include <boost/geometry/views/reversible_view.hpp>
#include <boost/geometry/geometries/segment.hpp>


namespace boost { namespace geometry
{


/*!
    \brief Structure containing section information
    \details Section information consists of a bounding box, direction
        information (if it is increasing or decreasing, per dimension),
        index information (begin-end, ring, multi) and the number of
        segments in this section

    \tparam Box box-type
    \tparam DimensionCount number of dimensions for this section
    \ingroup sectionalize
 */
template <typename Box, std::size_t DimensionCount>
struct section
{
    typedef Box box_type;

    int id; // might be obsolete now, BSG 14-03-2011 TODO decide about this

    int directions[DimensionCount];
    ring_identifier ring_id;
    Box bounding_box;

    int begin_index;
    int end_index;
    std::size_t count;
    std::size_t range_count;
    bool duplicate;
    int non_duplicate_index;

    inline section()
        : id(-1)
        , begin_index(-1)
        , end_index(-1)
        , count(0)
        , range_count(0)
        , duplicate(false)
        , non_duplicate_index(-1)
    {
        assign_inverse(bounding_box);
        for (std::size_t i = 0; i < DimensionCount; i++)
        {
            directions[i] = 0;
        }
    }
};


/*!
    \brief Structure containing a collection of sections
    \note Derived from a vector, proves to be faster than of deque
    \note vector might be templated in the future
    \ingroup sectionalize
 */
template <typename Box, std::size_t DimensionCount>
struct sections : std::vector<section<Box, DimensionCount> >
{
    typedef Box box_type;
    static std::size_t const value = DimensionCount;
};


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace sectionalize
{

template <typename Segment, std::size_t Dimension, std::size_t DimensionCount>
struct get_direction_loop
{
    typedef typename coordinate_type<Segment>::type coordinate_type;

    static inline void apply(Segment const& seg,
                int directions[DimensionCount])
    {
        coordinate_type const diff =
            geometry::get<1, Dimension>(seg) - geometry::get<0, Dimension>(seg);

        coordinate_type zero = coordinate_type();
        directions[Dimension] = diff > zero ? 1 : diff < zero ? -1 : 0;

        get_direction_loop
            <
                Segment, Dimension + 1, DimensionCount
            >::apply(seg, directions);
    }
};

template <typename Segment, std::size_t DimensionCount>
struct get_direction_loop<Segment, DimensionCount, DimensionCount>
{
    static inline void apply(Segment const&, int [DimensionCount])
    {}
};

template <typename T, std::size_t Dimension, std::size_t DimensionCount>
struct copy_loop
{
    static inline void apply(T const source[DimensionCount],
                T target[DimensionCount])
    {
        target[Dimension] = source[Dimension];
        copy_loop<T, Dimension + 1, DimensionCount>::apply(source, target);
    }
};

template <typename T, std::size_t DimensionCount>
struct copy_loop<T, DimensionCount, DimensionCount>
{
    static inline void apply(T const [DimensionCount], T [DimensionCount])
    {}
};

template <typename T, std::size_t Dimension, std::size_t DimensionCount>
struct compare_loop
{
    static inline bool apply(T const source[DimensionCount],
                T const target[DimensionCount])
    {
        bool const not_equal = target[Dimension] != source[Dimension];

        return not_equal
            ? false
            : compare_loop
                <
                    T, Dimension + 1, DimensionCount
                >::apply(source, target);
    }
};

template <typename T, std::size_t DimensionCount>
struct compare_loop<T, DimensionCount, DimensionCount>
{
    static inline bool apply(T const [DimensionCount],
                T const [DimensionCount])
    {

        return true;
    }
};


template <typename Segment, std::size_t Dimension, std::size_t DimensionCount>
struct check_duplicate_loop
{
    typedef typename coordinate_type<Segment>::type coordinate_type;

    static inline bool apply(Segment const& seg)
    {
        if (! geometry::math::equals
                (
                    geometry::get<0, Dimension>(seg), 
                    geometry::get<1, Dimension>(seg)
                )
            )
        {
            return false;
        }

        return check_duplicate_loop
            <
                Segment, Dimension + 1, DimensionCount
            >::apply(seg);
    }
};

template <typename Segment, std::size_t DimensionCount>
struct check_duplicate_loop<Segment, DimensionCount, DimensionCount>
{
    static inline bool apply(Segment const&)
    {
        return true;
    }
};

template <typename T, std::size_t Dimension, std::size_t DimensionCount>
struct assign_loop
{
    static inline void apply(T dims[DimensionCount], int const value)
    {
        dims[Dimension] = value;
        assign_loop<T, Dimension + 1, DimensionCount>::apply(dims, value);
    }
};

template <typename T, std::size_t DimensionCount>
struct assign_loop<T, DimensionCount, DimensionCount>
{
    static inline void apply(T [DimensionCount], int const)
    {
    }
};

/// @brief Helper class to create sections of a part of a range, on the fly
template
<
    typename Range,  // Can be closeable_view
    typename Point,
    typename Sections,
    std::size_t DimensionCount,
    std::size_t MaxCount
>
struct sectionalize_part
{
    typedef model::referring_segment<Point const> segment_type;
    typedef typename boost::range_value<Sections>::type section_type;

    typedef typename boost::range_iterator<Range const>::type iterator_type;

    static inline void apply(Sections& sections, section_type& section,
                int& index, int& ndi,
                Range const& range,
                ring_identifier ring_id)
    {
        if (int(boost::size(range)) <= index)
        {
            return;
        }

        if (index == 0)
        {
            ndi = 0;
        }

        iterator_type it = boost::begin(range);
        it += index;

        for(iterator_type previous = it++;
            it != boost::end(range);
            ++previous, ++it, index++)
        {
            segment_type segment(*previous, *it);

            int direction_classes[DimensionCount] = {0};
            get_direction_loop
                <
                    segment_type, 0, DimensionCount
                >::apply(segment, direction_classes);

            // if "dir" == 0 for all point-dimensions, it is duplicate.
            // Those sections might be omitted, if wished, lateron
            bool duplicate = false;

            if (direction_classes[0] == 0)
            {
                // Recheck because ALL dimensions should be checked,
                // not only first one.
                // (DimensionCount might be < dimension<P>::value)
                if (check_duplicate_loop
                    <
                        segment_type, 0, geometry::dimension<Point>::type::value
                    >::apply(segment)
                    )
                {
                    duplicate = true;

                    // Change direction-info to force new section
                    // Note that wo consecutive duplicate segments will generate
                    // only one duplicate-section.
                    // Actual value is not important as long as it is not -1,0,1
                    assign_loop
                    <
                        int, 0, DimensionCount
                    >::apply(direction_classes, -99);
                }
            }

            if (section.count > 0
                && (!compare_loop
                        <
                            int, 0, DimensionCount
                        >::apply(direction_classes, section.directions)
                    || section.count > MaxCount
                    )
                )
            {
                sections.push_back(section);
                section = section_type();
            }

            if (section.count == 0)
            {
                section.begin_index = index;
                section.ring_id = ring_id;
                section.duplicate = duplicate;
                section.non_duplicate_index = ndi;
                section.range_count = boost::size(range);

                copy_loop
                    <
                        int, 0, DimensionCount
                    >::apply(direction_classes, section.directions);
                geometry::expand(section.bounding_box, *previous);
            }

            geometry::expand(section.bounding_box, *it);
            section.end_index = index + 1;
            section.count++;
            if (! duplicate)
            {
                ndi++;
            }
        }
    }
};


template
<
    typename Range, closure_selector Closure, bool Reverse,
    typename Point,
    typename Sections,
    std::size_t DimensionCount,
    std::size_t MaxCount
>
struct sectionalize_range
{
    typedef typename closeable_view<Range const, Closure>::type cview_type;
    typedef typename reversible_view
        <
            cview_type const,
            Reverse ? iterate_reverse : iterate_forward
        >::type view_type;

    static inline void apply(Range const& range, Sections& sections,
                ring_identifier ring_id)
    {
        cview_type cview(range);
        view_type view(cview);

        std::size_t const n = boost::size(view);
        if (n == 0)
        {
            // Zero points, no section
            return;
        }

        if (n == 1)
        {
            // Line with one point ==> no sections
            return;
        }

        int index = 0;
        int ndi = 0; // non duplicate index

        typedef typename boost::range_value<Sections>::type section_type;
        section_type section;

        sectionalize_part
            <
                view_type, Point, Sections,
                DimensionCount, MaxCount
            >::apply(sections, section, index, ndi,
                        view, ring_id);

        // Add last section if applicable
        if (section.count > 0)
        {
            sections.push_back(section);
        }
    }
};

template
<
    typename Polygon,
    bool Reverse,
    typename Sections,
    std::size_t DimensionCount,
    std::size_t MaxCount
>
struct sectionalize_polygon
{
    static inline void apply(Polygon const& poly, Sections& sections,
                ring_identifier ring_id)
    {
        typedef typename point_type<Polygon>::type point_type;
        typedef typename ring_type<Polygon>::type ring_type;
        typedef sectionalize_range
            <
                ring_type, closure<Polygon>::value, Reverse,
                point_type, Sections, DimensionCount, MaxCount
            > sectionalizer_type;

        ring_id.ring_index = -1;
        sectionalizer_type::apply(exterior_ring(poly), sections, ring_id);//-1, multi_index);

        ring_id.ring_index++;
        typename interior_return_type<Polygon const>::type rings
                    = interior_rings(poly);
        for (BOOST_AUTO_TPL(it, boost::begin(rings)); it != boost::end(rings);
             ++it, ++ring_id.ring_index)
        {
            sectionalizer_type::apply(*it, sections, ring_id);
        }
    }
};

template
<
    typename Box,
    typename Sections,
    std::size_t DimensionCount,
    std::size_t MaxCount
>
struct sectionalize_box
{
    static inline void apply(Box const& box, Sections& sections, ring_identifier const& ring_id)
    {
        typedef typename point_type<Box>::type point_type;

        assert_dimension<Box, 2>();

        // Add all four sides of the 2D-box as separate section.
        // Easiest is to convert it to a polygon.
        // However, we don't have the polygon type
        // (or polygon would be a helper-type).
        // Therefore we mimic a linestring/std::vector of 5 points

        // TODO: might be replaced by assign_box_corners_oriented 
        // or just "convert"
        point_type ll, lr, ul, ur;
        geometry::detail::assign_box_corners(box, ll, lr, ul, ur);

        std::vector<point_type> points;
        points.push_back(ll);
        points.push_back(ul);
        points.push_back(ur);
        points.push_back(lr);
        points.push_back(ll);

        sectionalize_range
            <
                std::vector<point_type>, closed, false,
                point_type,
                Sections,
                DimensionCount,
                MaxCount
            >::apply(points, sections, ring_id);
    }
};

template <typename Sections>
inline void set_section_unique_ids(Sections& sections)
{
    // Set ID's.
    int index = 0;
    for (typename boost::range_iterator<Sections>::type it = boost::begin(sections);
        it != boost::end(sections);
        ++it)
    {
        it->id = index++;
    }
}

template <typename Sections>
inline void enlargeSections(Sections& sections)
{
    // Robustness issue. Increase sections a tiny bit such that all points are really within (and not on border)
    // Reason: turns might, rarely, be missed otherwise (case: "buffer_mp1")
    // Drawback: not really, range is now completely inside the section. Section is a tiny bit too large,
    // which might cause (a small number) of more comparisons
    // TODO: make dimension-agnostic
    for (typename boost::range_iterator<Sections>::type it = boost::begin(sections);
        it != boost::end(sections);
        ++it)
    {
        typedef typename boost::range_value<Sections>::type section_type;
        typedef typename section_type::box_type box_type;
        typedef typename geometry::coordinate_type<box_type>::type coordinate_type;
        coordinate_type const reps = math::relaxed_epsilon(10.0);
        geometry::set<0, 0>(it->bounding_box, geometry::get<0, 0>(it->bounding_box) - reps);
        geometry::set<0, 1>(it->bounding_box, geometry::get<0, 1>(it->bounding_box) - reps);
        geometry::set<1, 0>(it->bounding_box, geometry::get<1, 0>(it->bounding_box) + reps);
        geometry::set<1, 1>(it->bounding_box, geometry::get<1, 1>(it->bounding_box) + reps);
    }
}


}} // namespace detail::sectionalize
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Tag,
    typename Geometry,
    bool Reverse,
    typename Sections,
    std::size_t DimensionCount,
    std::size_t MaxCount
>
struct sectionalize
{
    BOOST_MPL_ASSERT_MSG
        (
            false, NOT_OR_NOT_YET_IMPLEMENTED_FOR_THIS_GEOMETRY_TYPE
            , (types<Geometry>)
        );
};

template
<
    typename Box,
    bool Reverse,
    typename Sections,
    std::size_t DimensionCount,
    std::size_t MaxCount
>
struct sectionalize<box_tag, Box, Reverse, Sections, DimensionCount, MaxCount>
    : detail::sectionalize::sectionalize_box
        <
            Box,
            Sections,
            DimensionCount,
            MaxCount
        >
{};

template
<
    typename LineString,
    typename Sections,
    std::size_t DimensionCount,
    std::size_t MaxCount
>
struct sectionalize
    <
        linestring_tag,
        LineString,
        false,
        Sections,
        DimensionCount,
        MaxCount
    >
    : detail::sectionalize::sectionalize_range
        <
            LineString, closed, false,
            typename point_type<LineString>::type,
            Sections,
            DimensionCount,
            MaxCount
        >
{};

template
<
    typename Ring,
    bool Reverse,
    typename Sections,
    std::size_t DimensionCount,
    std::size_t MaxCount
>
struct sectionalize<ring_tag, Ring, Reverse, Sections, DimensionCount, MaxCount>
    : detail::sectionalize::sectionalize_range
        <
            Ring, geometry::closure<Ring>::value, Reverse,
            typename point_type<Ring>::type,
            Sections,
            DimensionCount,
            MaxCount
        >
{};

template
<
    typename Polygon,
    bool Reverse,
    typename Sections,
    std::size_t DimensionCount,
    std::size_t MaxCount
>
struct sectionalize<polygon_tag, Polygon, Reverse, Sections, DimensionCount, MaxCount>
    : detail::sectionalize::sectionalize_polygon
        <
            Polygon, Reverse, Sections, DimensionCount, MaxCount
        >
{};

} // namespace dispatch
#endif


/*!
    \brief Split a geometry into monotonic sections
    \ingroup sectionalize
    \tparam Geometry type of geometry to check
    \tparam Sections type of sections to create
    \param geometry geometry to create sections from
    \param sections structure with sections
    \param source_index index to assign to the ring_identifiers
 */
template<bool Reverse, typename Geometry, typename Sections>
inline void sectionalize(Geometry const& geometry, Sections& sections, int source_index = 0)
{
    concept::check<Geometry const>();

    // TODO: review use of this constant (see below) as causing problems with GCC 4.6 --mloskot
    // A maximum of 10 segments per section seems to give the fastest results
    //static std::size_t const max_segments_per_section = 10;
    typedef dispatch::sectionalize
        <
            typename tag<Geometry>::type,
            Geometry,
            Reverse,
            Sections,
            Sections::value,
            10 // TODO: max_segments_per_section
        > sectionalizer_type;

    sections.clear();
    ring_identifier ring_id;
    ring_id.source_index = source_index;
    sectionalizer_type::apply(geometry, sections, ring_id);
    detail::sectionalize::set_section_unique_ids(sections);
    detail::sectionalize::enlargeSections(sections);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_SECTIONS_SECTIONALIZE_HPP
