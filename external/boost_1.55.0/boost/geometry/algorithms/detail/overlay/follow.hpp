// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_FOLLOW_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_FOLLOW_HPP

#include <cstddef>

#include <boost/range.hpp>
#include <boost/mpl/assert.hpp>

#include <boost/geometry/algorithms/detail/point_on_border.hpp>
#include <boost/geometry/algorithms/detail/overlay/append_no_duplicates.hpp>
#include <boost/geometry/algorithms/detail/overlay/copy_segments.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>

#include <boost/geometry/algorithms/covered_by.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

namespace following
{
    
template <typename Turn, typename Operation>
static inline bool is_entering(Turn const& /* TODO remove this parameter */, Operation const& op)
{
    // (Blocked means: blocked for polygon/polygon intersection, because
    // they are reversed. But for polygon/line it is similar to continue)
    return op.operation == operation_intersection
        || op.operation == operation_continue
        || op.operation == operation_blocked
        ;
}

template 
<
    typename Turn, 
    typename Operation, 
    typename LineString, 
    typename Polygon
>
static inline bool last_covered_by(Turn const& turn, Operation const& op, 
                LineString const& linestring, Polygon const& polygon)
{
    // Check any point between the this one and the first IP 
    typedef typename geometry::point_type<LineString>::type point_type;
    point_type point_in_between;
    detail::point_on_border::midpoint_helper
        <
            point_type,
            0, dimension<point_type>::value
        >::apply(point_in_between, linestring[op.seg_id.segment_index], turn.point);

    return geometry::covered_by(point_in_between, polygon);
}


template 
<
    typename Turn, 
    typename Operation, 
    typename LineString, 
    typename Polygon
>
static inline bool is_leaving(Turn const& turn, Operation const& op, 
                bool entered, bool first, 
                LineString const& linestring, Polygon const& polygon)
{
    if (op.operation == operation_union)
    {
        return entered 
            || turn.method == method_crosses
            || (first && last_covered_by(turn, op, linestring, polygon))
            ;
    }
    return false;
}


template 
<
    typename Turn, 
    typename Operation, 
    typename LineString, 
    typename Polygon
>
static inline bool is_staying_inside(Turn const& turn, Operation const& op, 
                bool entered, bool first, 
                LineString const& linestring, Polygon const& polygon)
{
    if (turn.method == method_crosses)
    {
        // The normal case, this is completely covered with entering/leaving 
        // so stay out of this time consuming "covered_by"
        return false;
    }

    if (is_entering(turn, op))
    {
        return entered || (first && last_covered_by(turn, op, linestring, polygon));
    }

    return false;
}

template 
<
    typename Turn, 
    typename Operation, 
    typename Linestring, 
    typename Polygon
>
static inline bool was_entered(Turn const& turn, Operation const& op, bool first,
                Linestring const& linestring, Polygon const& polygon)
{
    if (first && (turn.method == method_collinear || turn.method == method_equal))
    {
        return last_covered_by(turn, op, linestring, polygon);
    }
    return false;
}


// Template specialization structure to call the right actions for the right type
template<overlay_type OverlayType>
struct action_selector
{
    // If you get here the overlay type is not intersection or difference
    // BOOST_MPL_ASSERT(false);
};

// Specialization for intersection, containing the implementation
template<>
struct action_selector<overlay_intersection>
{
    template
    <
        typename OutputIterator, 
        typename LineStringOut, 
        typename LineString, 
        typename Point, 
        typename Operation
    >
    static inline void enter(LineStringOut& current_piece,
                LineString const& , 
                segment_identifier& segment_id,
                int , Point const& point,
                Operation const& operation, OutputIterator& )
    {
        // On enter, append the intersection point and remember starting point
        detail::overlay::append_no_duplicates(current_piece, point);
        segment_id = operation.seg_id;
    }

    template
    <
        typename OutputIterator, 
        typename LineStringOut, 
        typename LineString, 
        typename Point, 
        typename Operation
    >
    static inline void leave(LineStringOut& current_piece,
                LineString const& linestring,
                segment_identifier& segment_id,
                int index, Point const& point,
                Operation const& , OutputIterator& out)
    {
        // On leave, copy all segments from starting point, append the intersection point
        // and add the output piece
        geometry::copy_segments<false>(linestring, segment_id, index, current_piece);
        detail::overlay::append_no_duplicates(current_piece, point);
        if (current_piece.size() > 1)
        {
            *out++ = current_piece;
        }
        current_piece.clear();
    }

    static inline bool is_entered(bool entered)
    {
        return entered;
    }

    template <typename Point, typename Geometry>
    static inline bool included(Point const& point, Geometry const& geometry)
    {
        return geometry::covered_by(point, geometry);
    }

};

// Specialization for difference, which reverses these actions
template<>
struct action_selector<overlay_difference>
{
    typedef action_selector<overlay_intersection> normal_action;

    template
    <
        typename OutputIterator, 
        typename LineStringOut, 
        typename LineString, 
        typename Point, 
        typename Operation
    >
    static inline void enter(LineStringOut& current_piece, 
                LineString const& linestring, 
                segment_identifier& segment_id, 
                int index, Point const& point, 
                Operation const& operation, OutputIterator& out)
    {
        normal_action::leave(current_piece, linestring, segment_id, index, 
                    point, operation, out);
    }

    template
    <
        typename OutputIterator, 
        typename LineStringOut, 
        typename LineString, 
        typename Point, 
        typename Operation
    >
    static inline void leave(LineStringOut& current_piece,
                LineString const& linestring,
                segment_identifier& segment_id,
                int index, Point const& point,
                Operation const& operation, OutputIterator& out)
    {
        normal_action::enter(current_piece, linestring, segment_id, index,
                    point, operation, out);
    }

    static inline bool is_entered(bool entered)
    {
        return ! normal_action::is_entered(entered);
    }

    template <typename Point, typename Geometry>
    static inline bool included(Point const& point, Geometry const& geometry)
    {
        return ! normal_action::included(point, geometry);
    }

};

}

/*!
\brief Follows a linestring from intersection point to intersection point, outputting which
    is inside, or outside, a ring or polygon
\ingroup overlay
 */
template
<
    typename LineStringOut,
    typename LineString,
    typename Polygon,
    overlay_type OverlayType
>
class follow
{

    template<typename Turn>
    struct sort_on_segment
    {
        // In case of turn point at the same location, we want to have continue/blocked LAST
        // because that should be followed (intersection) or skipped (difference).
        inline int operation_order(Turn const& turn) const
        {
            operation_type const& operation = turn.operations[0].operation;
            switch(operation)
            {
                case operation_opposite : return 0;
                case operation_none : return 0;
                case operation_union : return 1;
                case operation_intersection : return 2;
                case operation_blocked : return 3;
                case operation_continue : return 4;
            }
            return -1;
        };

        inline bool use_operation(Turn const& left, Turn const& right) const
        {
            // If they are the same, OK. 
            return operation_order(left) < operation_order(right);
        }

        inline bool use_distance(Turn const& left, Turn const& right) const
        {
            return geometry::math::equals(left.operations[0].enriched.distance, right.operations[0].enriched.distance)
                ? use_operation(left, right)
                : left.operations[0].enriched.distance < right.operations[0].enriched.distance
                ;
        }

        inline bool operator()(Turn const& left, Turn const& right) const
        {
            segment_identifier const& sl = left.operations[0].seg_id;
            segment_identifier const& sr = right.operations[0].seg_id;

            return sl == sr
                ? use_distance(left, right)
                : sl < sr
                ;

        }
    };



public :

    template <typename Point, typename Geometry>
    static inline bool included(Point const& point, Geometry const& geometry)
    {
        return following::action_selector<OverlayType>::included(point, geometry);
    }

    template<typename Turns, typename OutputIterator>
    static inline OutputIterator apply(LineString const& linestring, Polygon const& polygon,
                detail::overlay::operation_type ,  // TODO: this parameter might be redundant
                Turns& turns, OutputIterator out)
    {
        typedef typename boost::range_iterator<Turns>::type turn_iterator;
        typedef typename boost::range_value<Turns>::type turn_type;
        typedef typename boost::range_iterator
            <
                typename turn_type::container_type
            >::type turn_operation_iterator_type;

        typedef following::action_selector<OverlayType> action;

        // Sort intersection points on segments-along-linestring, and distance
        // (like in enrich is done for poly/poly)
        std::sort(boost::begin(turns), boost::end(turns), sort_on_segment<turn_type>());

        LineStringOut current_piece;
        geometry::segment_identifier current_segment_id(0, -1, -1, -1);

        // Iterate through all intersection points (they are ordered along the line)
        bool entered = false;
        bool first = true;
        for (turn_iterator it = boost::begin(turns); it != boost::end(turns); ++it)
        {
            turn_operation_iterator_type iit = boost::begin(it->operations);

            if (following::was_entered(*it, *iit, first, linestring, polygon))
            {
                debug_traverse(*it, *iit, "-> Was entered");
                entered = true;
            }

            if (following::is_staying_inside(*it, *iit, entered, first, linestring, polygon))
            {
                debug_traverse(*it, *iit, "-> Staying inside");

                entered = true;
            }
            else if (following::is_entering(*it, *iit))
            {
                debug_traverse(*it, *iit, "-> Entering");

                entered = true;
                action::enter(current_piece, linestring, current_segment_id, iit->seg_id.segment_index, it->point, *iit, out);
            }
            else if (following::is_leaving(*it, *iit, entered, first, linestring, polygon))
            {
                debug_traverse(*it, *iit, "-> Leaving");

                entered = false;
                action::leave(current_piece, linestring, current_segment_id, iit->seg_id.segment_index, it->point, *iit, out);
            }
            first = false;
        }

        if (action::is_entered(entered))
        {
            geometry::copy_segments<false>(linestring, current_segment_id,
                    boost::size(linestring) - 1,
                    current_piece);
        }

        // Output the last one, if applicable
        if (current_piece.size() > 1)
        {
            *out++ = current_piece;
        }
        return out;
    }

};


}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_FOLLOW_HPP
