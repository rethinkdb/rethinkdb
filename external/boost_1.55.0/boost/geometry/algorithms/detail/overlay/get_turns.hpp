// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURNS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURNS_HPP


#include <cstddef>
#include <map>

#include <boost/array.hpp>
#include <boost/concept_check.hpp>
#include <boost/mpl/if.hpp>
#include <boost/range.hpp>
#include <boost/typeof/typeof.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/reverse_dispatch.hpp>

#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/ring_type.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/views/closeable_view.hpp>
#include <boost/geometry/views/reversible_view.hpp>
#include <boost/geometry/views/detail/range_type.hpp>

#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/segment.hpp>

#include <boost/geometry/iterators/ever_circling_iterator.hpp>

#include <boost/geometry/strategies/cartesian/cart_intersect.hpp>
#include <boost/geometry/strategies/intersection.hpp>
#include <boost/geometry/strategies/intersection_result.hpp>

#include <boost/geometry/algorithms/detail/disjoint.hpp>
#include <boost/geometry/algorithms/detail/partition.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turn_info.hpp>

#include <boost/geometry/algorithms/detail/overlay/segment_identifier.hpp>


#include <boost/geometry/algorithms/detail/sections/range_by_section.hpp>

#include <boost/geometry/algorithms/expand.hpp>
#include <boost/geometry/algorithms/detail/sections/sectionalize.hpp>

#ifdef BOOST_GEOMETRY_DEBUG_INTERSECTION
#  include <sstream>
#  include <boost/geometry/io/dsv/write.hpp>
#endif


namespace boost { namespace geometry
{

// Silence warning C4127: conditional expression is constant
#if defined(_MSC_VER)
#pragma warning(push)  
#pragma warning(disable : 4127)  
#endif
    

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace get_turns
{


struct no_interrupt_policy
{
    static bool const enabled = false;

    template <typename Range>
    static inline bool apply(Range const&)
    {
        return false;
    }
};


template
<
    typename Geometry1, typename Geometry2,
    bool Reverse1, bool Reverse2,
    typename Section1, typename Section2,
    typename Turns,
    typename TurnPolicy,
    typename InterruptPolicy
>
class get_turns_in_sections
{
    typedef typename closeable_view
        <
            typename range_type<Geometry1>::type const,
            closure<Geometry1>::value
        >::type cview_type1;
    typedef typename closeable_view
        <
            typename range_type<Geometry2>::type const,
            closure<Geometry2>::value
        >::type cview_type2;

    typedef typename reversible_view
        <
            cview_type1 const,
            Reverse1 ? iterate_reverse : iterate_forward
        >::type view_type1;
    typedef typename reversible_view
        <
            cview_type2 const,
            Reverse2 ? iterate_reverse : iterate_forward
        >::type view_type2;

    typedef typename boost::range_iterator
        <
            view_type1 const
        >::type range1_iterator;

    typedef typename boost::range_iterator
        <
            view_type2 const
        >::type range2_iterator;


    template <typename Geometry, typename Section>
    static inline bool neighbouring(Section const& section,
            int index1, int index2)
    {
        // About n-2:
        //   (square: range_count=5, indices 0,1,2,3
        //    -> 0-3 are adjacent, don't check on intersections)
        // Also tested for open polygons, and/or duplicates
        // About first condition: will be optimized by compiler (static)
        // It checks if it is areal (box,ring,(multi)polygon
        int const n = int(section.range_count);

        boost::ignore_unused_variable_warning(n);
        boost::ignore_unused_variable_warning(index1);
        boost::ignore_unused_variable_warning(index2);

        return boost::is_same
                    <
                        typename tag_cast
                            <
                                typename geometry::point_type<Geometry1>::type, 
                                areal_tag
                            >::type, 
                        areal_tag
                    >::value
               && index1 == 0 
               && index2 >= n - 2
                ;
    }


public :
    // Returns true if terminated, false if interrupted
    static inline bool apply(
            int source_id1, Geometry1 const& geometry1, Section1 const& sec1,
            int source_id2, Geometry2 const& geometry2, Section2 const& sec2,
            bool skip_larger,
            Turns& turns,
            InterruptPolicy& interrupt_policy)
    {
        boost::ignore_unused_variable_warning(interrupt_policy);

        cview_type1 cview1(range_by_section(geometry1, sec1));
        cview_type2 cview2(range_by_section(geometry2, sec2));
        view_type1 view1(cview1);
        view_type2 view2(cview2);

        range1_iterator begin_range_1 = boost::begin(view1);
        range1_iterator end_range_1 = boost::end(view1);

        range2_iterator begin_range_2 = boost::begin(view2);
        range2_iterator end_range_2 = boost::end(view2);

        int const dir1 = sec1.directions[0];
        int const dir2 = sec2.directions[0];
        int index1 = sec1.begin_index;
        int ndi1 = sec1.non_duplicate_index;

        bool const same_source =
            source_id1 == source_id2
                    && sec1.ring_id.multi_index == sec2.ring_id.multi_index
                    && sec1.ring_id.ring_index == sec2.ring_id.ring_index;

        range1_iterator prev1, it1, end1;

        get_start_point_iterator(sec1, view1, prev1, it1, end1,
                    index1, ndi1, dir1, sec2.bounding_box);

        // We need a circular iterator because it might run through the closing point.
        // One circle is actually enough but this one is just convenient.
        ever_circling_iterator<range1_iterator> next1(begin_range_1, end_range_1, it1, true);
        next1++;

        // Walk through section and stop if we exceed the other box
        // section 2:    [--------------]
        // section 1: |----|---|---|---|---|
        for (prev1 = it1++, next1++;
            it1 != end1 && ! exceeding<0>(dir1, *prev1, sec2.bounding_box);
            ++prev1, ++it1, ++index1, ++next1, ++ndi1)
        {
            ever_circling_iterator<range1_iterator> nd_next1(
                    begin_range_1, end_range_1, next1, true);
            advance_to_non_duplicate_next(nd_next1, it1, sec1);

            int index2 = sec2.begin_index;
            int ndi2 = sec2.non_duplicate_index;

            range2_iterator prev2, it2, end2;

            get_start_point_iterator(sec2, view2, prev2, it2, end2,
                        index2, ndi2, dir2, sec1.bounding_box);
            ever_circling_iterator<range2_iterator> next2(begin_range_2, end_range_2, it2, true);
            next2++;

            for (prev2 = it2++, next2++;
                it2 != end2 && ! exceeding<0>(dir2, *prev2, sec1.bounding_box);
                ++prev2, ++it2, ++index2, ++next2, ++ndi2)
            {
                bool skip = same_source;
                if (skip)
                {
                    // If sources are the same (possibly self-intersecting):
                    // skip if it is a neighbouring segment.
                    // (including first-last segment
                    //  and two segments with one or more degenerate/duplicate
                    //  (zero-length) segments in between)

                    // Also skip if index1 < index2 to avoid getting all
                    // intersections twice (only do this on same source!)

                    skip = (skip_larger && index1 >= index2)
                        || ndi2 == ndi1 + 1
                        || neighbouring<Geometry1>(sec1, index1, index2)
                        ;
                }

                if (! skip)
                {
                    // Move to the "non duplicate next"
                    ever_circling_iterator<range2_iterator> nd_next2(
                            begin_range_2, end_range_2, next2, true);
                    advance_to_non_duplicate_next(nd_next2, it2, sec2);

                    typedef typename boost::range_value<Turns>::type turn_info;

                    turn_info ti;
                    ti.operations[0].seg_id = segment_identifier(source_id1,
                                        sec1.ring_id.multi_index, sec1.ring_id.ring_index, index1),
                    ti.operations[1].seg_id = segment_identifier(source_id2,
                                        sec2.ring_id.multi_index, sec2.ring_id.ring_index, index2),

                    ti.operations[0].other_id = ti.operations[1].seg_id;
                    ti.operations[1].other_id = ti.operations[0].seg_id;

                    std::size_t const size_before = boost::size(turns);

                    TurnPolicy::apply(*prev1, *it1, *nd_next1, *prev2, *it2, *nd_next2,
                            ti, std::back_inserter(turns));

                    if (InterruptPolicy::enabled)
                    {
                        if (interrupt_policy.apply(
                            std::make_pair(boost::begin(turns) + size_before,
                                boost::end(turns))))
                        {
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }


private :
    typedef typename geometry::point_type<Geometry1>::type point1_type;
    typedef typename geometry::point_type<Geometry2>::type point2_type;
    typedef typename model::referring_segment<point1_type const> segment1_type;
    typedef typename model::referring_segment<point2_type const> segment2_type;


    template <size_t Dim, typename Point, typename Box>
    static inline bool preceding(int dir, Point const& point, Box const& box)
    {
        return (dir == 1  && get<Dim>(point) < get<min_corner, Dim>(box))
            || (dir == -1 && get<Dim>(point) > get<max_corner, Dim>(box));
    }

    template <size_t Dim, typename Point, typename Box>
    static inline bool exceeding(int dir, Point const& point, Box const& box)
    {
        return (dir == 1  && get<Dim>(point) > get<max_corner, Dim>(box))
            || (dir == -1 && get<Dim>(point) < get<min_corner, Dim>(box));
    }

    template <typename Iterator, typename RangeIterator, typename Section>
    static inline void advance_to_non_duplicate_next(Iterator& next,
            RangeIterator const& it, Section const& section)
    {
        // To see where the next segments bend to, in case of touch/intersections
        // on end points, we need (in case of degenerate/duplicate points) an extra
        // iterator which moves to the REAL next point, so non duplicate.
        // This needs an extra comparison (disjoint).
        // (Note that within sections, non duplicate points are already asserted,
        //   by the sectionalize process).

        // So advance to the "non duplicate next"
        // (the check is defensive, to avoid endless loops)
        std::size_t check = 0;
        while(! detail::disjoint::disjoint_point_point(*it, *next)
            && check++ < section.range_count)
        {
            next++;
        }
    }

    // It is NOT possible to have section-iterators here
    // because of the logistics of "index" (the section-iterator automatically
    // skips to the begin-point, we loose the index or have to recalculate it)
    // So we mimic it here
    template <typename Range, typename Section, typename Box>
    static inline void get_start_point_iterator(Section & section,
            Range const& range,
            typename boost::range_iterator<Range const>::type& it,
            typename boost::range_iterator<Range const>::type& prev,
            typename boost::range_iterator<Range const>::type& end,
            int& index, int& ndi,
            int dir, Box const& other_bounding_box)
    {
        it = boost::begin(range) + section.begin_index;
        end = boost::begin(range) + section.end_index + 1;

        // Mimic section-iterator:
        // Skip to point such that section interects other box
        prev = it++;
        for(; it != end && preceding<0>(dir, *it, other_bounding_box);
            prev = it++, index++, ndi++)
        {}
        // Go back one step because we want to start completely preceding
        it = prev;
    }
};

struct get_section_box
{
    template <typename Box, typename InputItem>
    static inline void apply(Box& total, InputItem const& item)
    {
        geometry::expand(total, item.bounding_box);
    }
};

struct ovelaps_section_box
{
    template <typename Box, typename InputItem>
    static inline bool apply(Box const& box, InputItem const& item)
    {
        return ! detail::disjoint::disjoint_box_box(box, item.bounding_box);
    }
};

template
<
    typename Geometry1, typename Geometry2,
    bool Reverse1, bool Reverse2,
    typename Turns,
    typename TurnPolicy,
    typename InterruptPolicy
>
struct section_visitor
{
    int m_source_id1;
    Geometry1 const& m_geometry1;
    int m_source_id2;
    Geometry2 const& m_geometry2;
    Turns& m_turns;
    InterruptPolicy& m_interrupt_policy;

    section_visitor(int id1, Geometry1 const& g1,
            int id2, Geometry2 const& g2,
            Turns& turns, InterruptPolicy& ip)
        : m_source_id1(id1), m_geometry1(g1)
        , m_source_id2(id2), m_geometry2(g2)
        , m_turns(turns)
        , m_interrupt_policy(ip)
    {}

    template <typename Section>
    inline bool apply(Section const& sec1, Section const& sec2)
    {
        if (! detail::disjoint::disjoint_box_box(sec1.bounding_box, sec2.bounding_box))
        {
            return get_turns_in_sections
                    <
                        Geometry1,
                        Geometry2,
                        Reverse1, Reverse2,
                        Section, Section,
                        Turns,
                        TurnPolicy,
                        InterruptPolicy
                    >::apply(
                            m_source_id1, m_geometry1, sec1,
                            m_source_id2, m_geometry2, sec2,
                            false,
                            m_turns, m_interrupt_policy);
        }
        return true;
    }

};

template
<
    typename Geometry1, typename Geometry2,
    bool Reverse1, bool Reverse2,
    typename Turns,
    typename TurnPolicy,
    typename InterruptPolicy
>
class get_turns_generic
{

public:
    static inline void apply(
            int source_id1, Geometry1 const& geometry1,
            int source_id2, Geometry2 const& geometry2,
            Turns& turns, InterruptPolicy& interrupt_policy)
    {
        // First create monotonic sections...
        typedef typename boost::range_value<Turns>::type ip_type;
        typedef typename ip_type::point_type point_type;
        typedef model::box<point_type> box_type;
        typedef typename geometry::sections<box_type, 2> sections_type;

        sections_type sec1, sec2;

        geometry::sectionalize<Reverse1>(geometry1, sec1, 0);
        geometry::sectionalize<Reverse2>(geometry2, sec2, 1);

        // ... and then partition them, intersecting overlapping sections in visitor method
        section_visitor
            <
                Geometry1, Geometry2,
                Reverse1, Reverse2,
                Turns, TurnPolicy, InterruptPolicy
            > visitor(source_id1, geometry1, source_id2, geometry2, turns, interrupt_policy);

        geometry::partition
            <
                box_type, get_section_box, ovelaps_section_box
            >::apply(sec1, sec2, visitor);
    }
};


// Get turns for a range with a box, following Cohen-Sutherland (cs) approach
template
<
    typename Range, typename Box,
    bool ReverseRange, bool ReverseBox,
    typename Turns,
    typename TurnPolicy,
    typename InterruptPolicy
>
struct get_turns_cs
{
    typedef typename boost::range_value<Turns>::type turn_info;
    typedef typename geometry::point_type<Range>::type point_type;
    typedef typename geometry::point_type<Box>::type box_point_type;

    typedef typename closeable_view
        <
            Range const,
            closure<Range>::value
        >::type cview_type;

    typedef typename reversible_view
        <
            cview_type const,
            ReverseRange ? iterate_reverse : iterate_forward
        >::type view_type;

    typedef typename boost::range_iterator
        <
            view_type const
        >::type iterator_type;


    static inline void apply(
                int source_id1, Range const& range,
                int source_id2, Box const& box,
                Turns& turns,
                InterruptPolicy& interrupt_policy,
                int multi_index = -1, int ring_index = -1)
    {
        if (boost::size(range) <= 1)
        {
            return;
        }

        boost::array<box_point_type,4> bp;
        assign_box_corners_oriented<ReverseBox>(box, bp);

        cview_type cview(range);
        view_type view(cview);

        iterator_type it = boost::begin(view);

        ever_circling_iterator<iterator_type> next(
                boost::begin(view), boost::end(view), it, true);
        next++;
        next++;

        //bool first = true;

        //char previous_side[2] = {0, 0};

        int index = 0;

        for (iterator_type prev = it++;
            it != boost::end(view);
            prev = it++, next++, index++)
        {
            segment_identifier seg_id(source_id1,
                        multi_index, ring_index, index);

            /*if (first)
            {
                previous_side[0] = get_side<0>(box, *prev);
                previous_side[1] = get_side<1>(box, *prev);
            }

            char current_side[2];
            current_side[0] = get_side<0>(box, *it);
            current_side[1] = get_side<1>(box, *it);

            // There can NOT be intersections if
            // 1) EITHER the two points are lying on one side of the box (! 0 && the same)
            // 2) OR same in Y-direction
            // 3) OR all points are inside the box (0)
            if (! (
                (current_side[0] != 0 && current_side[0] == previous_side[0])
                || (current_side[1] != 0 && current_side[1] == previous_side[1])
                || (current_side[0] == 0
                        && current_side[1] == 0
                        && previous_side[0] == 0
                        && previous_side[1] == 0)
                  )
                )*/
            if (true)
            {
                get_turns_with_box(seg_id, source_id2,
                        *prev, *it, *next,
                        bp[0], bp[1], bp[2], bp[3],
                        turns, interrupt_policy);
                // Future performance enhancement: 
                // return if told by the interrupt policy 
            }
        }
    }

private:
    template<std::size_t Index, typename Point>
    static inline int get_side(Box const& box, Point const& point)
    {
        // Inside -> 0
        // Outside -> -1 (left/below) or 1 (right/above)
        // On border -> -2 (left/lower) or 2 (right/upper)
        // The only purpose of the value is to not be the same,
        // and to denote if it is inside (0)

        typename coordinate_type<Point>::type const& c = get<Index>(point);
        typename coordinate_type<Box>::type const& left = get<min_corner, Index>(box);
        typename coordinate_type<Box>::type const& right = get<max_corner, Index>(box);

        if (geometry::math::equals(c, left)) return -2;
        else if (geometry::math::equals(c, right)) return 2;
        else if (c < left) return -1;
        else if (c > right) return 1;
        else return 0;
    }

    static inline void get_turns_with_box(segment_identifier const& seg_id, int source_id2,
            // Points from a range:
            point_type const& rp0,
            point_type const& rp1,
            point_type const& rp2,
            // Points from the box
            box_point_type const& bp0,
            box_point_type const& bp1,
            box_point_type const& bp2,
            box_point_type const& bp3,
            // Output
            Turns& turns,
            InterruptPolicy& interrupt_policy)
    {
        boost::ignore_unused_variable_warning(interrupt_policy);

        // Depending on code some relations can be left out

        typedef typename boost::range_value<Turns>::type turn_info;

        turn_info ti;
        ti.operations[0].seg_id = seg_id;
        ti.operations[0].other_id = ti.operations[1].seg_id;
        ti.operations[1].other_id = seg_id;

        ti.operations[1].seg_id = segment_identifier(source_id2, -1, -1, 0);
        TurnPolicy::apply(rp0, rp1, rp2, bp0, bp1, bp2,
                ti, std::back_inserter(turns));

        ti.operations[1].seg_id = segment_identifier(source_id2, -1, -1, 1);
        TurnPolicy::apply(rp0, rp1, rp2, bp1, bp2, bp3,
                ti, std::back_inserter(turns));

        ti.operations[1].seg_id = segment_identifier(source_id2, -1, -1, 2);
        TurnPolicy::apply(rp0, rp1, rp2, bp2, bp3, bp0,
                ti, std::back_inserter(turns));

        ti.operations[1].seg_id = segment_identifier(source_id2, -1, -1, 3);
        TurnPolicy::apply(rp0, rp1, rp2, bp3, bp0, bp1,
                ti, std::back_inserter(turns));

        if (InterruptPolicy::enabled)
        {
            interrupt_policy.apply(turns);
        }

    }

};


template
<
    typename Polygon, typename Box,
    bool Reverse, bool ReverseBox,
    typename Turns,
    typename TurnPolicy,
    typename InterruptPolicy
>
struct get_turns_polygon_cs
{
    static inline void apply(
            int source_id1, Polygon const& polygon,
            int source_id2, Box const& box,
            Turns& turns, InterruptPolicy& interrupt_policy,
            int multi_index = -1)
    {
        typedef typename geometry::ring_type<Polygon>::type ring_type;

        typedef detail::get_turns::get_turns_cs
            <
                ring_type, Box,
                Reverse, ReverseBox,
                Turns,
                TurnPolicy,
                InterruptPolicy
            > intersector_type;

        intersector_type::apply(
                source_id1, geometry::exterior_ring(polygon),
                source_id2, box, turns, interrupt_policy,
                multi_index, -1);

        int i = 0;

        typename interior_return_type<Polygon const>::type rings
                    = interior_rings(polygon);
        for (BOOST_AUTO_TPL(it, boost::begin(rings)); it != boost::end(rings);
            ++it, ++i)
        {
            intersector_type::apply(
                    source_id1, *it,
                    source_id2, box, turns, interrupt_policy,
                    multi_index, i);
        }

    }
};

}} // namespace detail::get_turns
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

// Because this is "detail" method, and most implementations will use "generic",
// we take the freedom to derive it from "generic".
template
<
    typename GeometryTag1, typename GeometryTag2,
    typename Geometry1, typename Geometry2,
    bool Reverse1, bool Reverse2,
    typename Turns,
    typename TurnPolicy,
    typename InterruptPolicy
>
struct get_turns
    : detail::get_turns::get_turns_generic
        <
            Geometry1, Geometry2,
            Reverse1, Reverse2,
            Turns,
            TurnPolicy,
            InterruptPolicy
        >
{};


template
<
    typename Polygon, typename Box,
    bool ReversePolygon, bool ReverseBox,
    typename Turns,
    typename TurnPolicy,
    typename InterruptPolicy
>
struct get_turns
    <
        polygon_tag, box_tag,
        Polygon, Box,
        ReversePolygon, ReverseBox,
        Turns,
        TurnPolicy,
        InterruptPolicy
    > : detail::get_turns::get_turns_polygon_cs
            <
                Polygon, Box,
                ReversePolygon, ReverseBox,
                Turns, TurnPolicy, InterruptPolicy
            >
{};


template
<
    typename Ring, typename Box,
    bool ReverseRing, bool ReverseBox,
    typename Turns,
    typename TurnPolicy,
    typename InterruptPolicy
>
struct get_turns
    <
        ring_tag, box_tag,
        Ring, Box,
        ReverseRing, ReverseBox,
        Turns,
        TurnPolicy,
        InterruptPolicy
    > : detail::get_turns::get_turns_cs
            <
                Ring, Box, ReverseRing, ReverseBox,
                Turns, TurnPolicy, InterruptPolicy
            >

{};


template
<
    typename GeometryTag1, typename GeometryTag2,
    typename Geometry1, typename Geometry2,
    bool Reverse1, bool Reverse2,
    typename Turns,
    typename TurnPolicy,
    typename InterruptPolicy
>
struct get_turns_reversed
{
    static inline void apply(
            int source_id1, Geometry1 const& g1,
            int source_id2, Geometry2 const& g2,
            Turns& turns, InterruptPolicy& interrupt_policy)
    {
        get_turns
            <
                GeometryTag2, GeometryTag1,
                Geometry2, Geometry1,
                Reverse2, Reverse1,
                Turns, TurnPolicy,
                InterruptPolicy
            >::apply(source_id2, g2, source_id1, g1, turns, interrupt_policy);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH



/*!
\brief \brief_calc2{turn points}
\ingroup overlay
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam Turns type of turn-container (e.g. vector of "intersection/turn point"'s)
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param turns container which will contain turn points
\param interrupt_policy policy determining if process is stopped
    when intersection is found
 */
template
<
    bool Reverse1, bool Reverse2,
    typename AssignPolicy,
    typename Geometry1,
    typename Geometry2,
    typename Turns,
    typename InterruptPolicy
>
inline void get_turns(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            Turns& turns,
            InterruptPolicy& interrupt_policy)
{
    concept::check_concepts_and_equal_dimensions<Geometry1 const, Geometry2 const>();

    //typedef typename strategy_intersection
    //    <
    //        typename cs_tag<Geometry1>::type,
    //        Geometry1,
    //        Geometry2,
    //        typename boost::range_value<Turns>::type
    //    >::segment_intersection_strategy_type segment_intersection_strategy_type;

    typedef detail::overlay::get_turn_info
        <
            typename point_type<Geometry1>::type,
            typename point_type<Geometry2>::type,
            typename boost::range_value<Turns>::type,
            AssignPolicy
        > TurnPolicy;

    boost::mpl::if_c
        <
            reverse_dispatch<Geometry1, Geometry2>::type::value,
            dispatch::get_turns_reversed
            <
                typename tag<Geometry1>::type,
                typename tag<Geometry2>::type,
                Geometry1, Geometry2,
                Reverse1, Reverse2,
                Turns, TurnPolicy,
                InterruptPolicy
            >,
            dispatch::get_turns
            <
                typename tag<Geometry1>::type,
                typename tag<Geometry2>::type,
                Geometry1, Geometry2,
                Reverse1, Reverse2,
                Turns, TurnPolicy,
                InterruptPolicy
            >
        >::type::apply(
            0, geometry1,
            1, geometry2,
            turns, interrupt_policy);
}

#if defined(_MSC_VER)
#pragma warning(pop)  
#endif

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURNS_HPP
