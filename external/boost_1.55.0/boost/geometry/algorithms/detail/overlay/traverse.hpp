// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_TRAVERSE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_TRAVERSE_HPP

#include <cstddef>

#include <boost/range.hpp>

#include <boost/geometry/algorithms/detail/overlay/append_no_dups_or_spikes.hpp>
#include <boost/geometry/algorithms/detail/overlay/backtrack_check_si.hpp>
#include <boost/geometry/algorithms/detail/overlay/copy_segments.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>
#include <boost/geometry/algorithms/num_points.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>

#if defined(BOOST_GEOMETRY_DEBUG_INTERSECTION) \
    || defined(BOOST_GEOMETRY_OVERLAY_REPORT_WKT) \
    || defined(BOOST_GEOMETRY_DEBUG_TRAVERSE)
#  include <string>
#  include <boost/geometry/algorithms/detail/overlay/debug_turn_info.hpp>
#  include <boost/geometry/io/wkt/wkt.hpp>
#endif

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

template <typename Turn, typename Operation>
#ifdef BOOST_GEOMETRY_DEBUG_TRAVERSE
inline void debug_traverse(Turn const& turn, Operation op, 
                std::string const& header)
{
    std::cout << header
        << " at " << op.seg_id
        << " meth: " << method_char(turn.method)
        << " op: " << operation_char(op.operation)
        << " vis: " << visited_char(op.visited)
        << " of:  " << operation_char(turn.operations[0].operation)
        << operation_char(turn.operations[1].operation)
        << " " << geometry::wkt(turn.point)
        << std::endl;

    if (boost::contains(header, "Finished"))
    {
        std::cout << std::endl;
    }
}
#else
inline void debug_traverse(Turn const& , Operation, const char*)
{
}
#endif


template <typename Info, typename Turn>
inline void set_visited_for_continue(Info& info, Turn const& turn)
{
    // On "continue", set "visited" for ALL directions
    if (turn.operation == detail::overlay::operation_continue)
    {
        for (typename boost::range_iterator
            <
                typename Info::container_type
            >::type it = boost::begin(info.operations);
            it != boost::end(info.operations);
            ++it)
        {
            if (it->visited.none())
            {
                it->visited.set_visited();
            }
        }
    }
}


template
<
    bool Reverse1, bool Reverse2,
    typename GeometryOut,
    typename G1,
    typename G2,
    typename Turns,
    typename IntersectionInfo
>
inline bool assign_next_ip(G1 const& g1, G2 const& g2,
            Turns& turns,
            typename boost::range_iterator<Turns>::type& ip,
            GeometryOut& current_output,
            IntersectionInfo& info,
            segment_identifier& seg_id)
{
    info.visited.set_visited();
    set_visited_for_continue(*ip, info);

    // If there is no next IP on this segment
    if (info.enriched.next_ip_index < 0)
    {
        if (info.enriched.travels_to_vertex_index < 0 
            || info.enriched.travels_to_ip_index < 0)
        {
            return false;
        }

        BOOST_ASSERT(info.enriched.travels_to_vertex_index >= 0);
        BOOST_ASSERT(info.enriched.travels_to_ip_index >= 0);

        if (info.seg_id.source_index == 0)
        {
            geometry::copy_segments<Reverse1>(g1, info.seg_id,
                    info.enriched.travels_to_vertex_index,
                    current_output);
        }
        else
        {
            geometry::copy_segments<Reverse2>(g2, info.seg_id,
                    info.enriched.travels_to_vertex_index,
                    current_output);
        }
        seg_id = info.seg_id;
        ip = boost::begin(turns) + info.enriched.travels_to_ip_index;
    }
    else
    {
        ip = boost::begin(turns) + info.enriched.next_ip_index;
        seg_id = info.seg_id;
    }

    detail::overlay::append_no_dups_or_spikes(current_output, ip->point);

    return true;
}


inline bool select_source(operation_type operation, int source1, int source2)
{
    return (operation == operation_intersection && source1 != source2)
        || (operation == operation_union && source1 == source2)
        ;
}


template
<
    typename Turn,
    typename Iterator
>
inline bool select_next_ip(operation_type operation,
            Turn& turn,
            segment_identifier const& seg_id,
            Iterator& selected)
{
    if (turn.discarded)
    {
        return false;
    }
    bool has_tp = false;
    selected = boost::end(turn.operations);
    for (Iterator it = boost::begin(turn.operations);
        it != boost::end(turn.operations);
        ++it)
    {
        if (it->visited.started())
        {
            selected = it;
            //std::cout << " RETURN";
            return true;
        }

        // In some cases there are two alternatives.
        // For "ii", take the other one (alternate)
        //           UNLESS the other one is already visited
        // For "uu", take the same one (see above);
        // For "cc", take either one, but if there is a starting one,
        //           take that one.
        if (   (it->operation == operation_continue
                && (! has_tp || it->visited.started()
                    )
                )
            || (it->operation == operation
                && ! it->visited.finished()
                && (! has_tp
                    || select_source(operation,
                            it->seg_id.source_index, seg_id.source_index)
                    )
                )
            )
        {
            selected = it;
            debug_traverse(turn, *it, " Candidate");
            has_tp = true;
        }
    }

    if (has_tp)
    {
       debug_traverse(turn, *selected, "  Accepted");
    }


    return has_tp;
}



/*!
    \brief Traverses through intersection points / geometries
    \ingroup overlay
 */
template
<
    bool Reverse1, bool Reverse2,
    typename Geometry1,
    typename Geometry2,
    typename Backtrack = backtrack_check_self_intersections<Geometry1, Geometry2>
>
class traverse
{
public :
    template <typename Turns, typename Rings>
    static inline void apply(Geometry1 const& geometry1,
                Geometry2 const& geometry2,
                detail::overlay::operation_type operation,
                Turns& turns, Rings& rings)
    {
        typedef typename boost::range_value<Rings>::type ring_type;
        typedef typename boost::range_iterator<Turns>::type turn_iterator;
        typedef typename boost::range_value<Turns>::type turn_type;
        typedef typename boost::range_iterator
            <
                typename turn_type::container_type
            >::type turn_operation_iterator_type;

        std::size_t const min_num_points
                = core_detail::closure::minimum_ring_size
                        <
                            geometry::closure<ring_type>::value
                        >::value;

        std::size_t size_at_start = boost::size(rings);

        typename Backtrack::state_type state;
        do
        {
            state.reset();

            // Iterate through all unvisited points
            for (turn_iterator it = boost::begin(turns);
                state.good() && it != boost::end(turns);
                ++it)
            {
                // Skip discarded ones
                if (! (it->is_discarded() || it->blocked()))
                {
                    for (turn_operation_iterator_type iit = boost::begin(it->operations);
                        state.good() && iit != boost::end(it->operations);
                        ++iit)
                    {
                        if (iit->visited.none()
                            && ! iit->visited.rejected()
                            && (iit->operation == operation
                                || iit->operation == detail::overlay::operation_continue)
                            )
                        {
                            set_visited_for_continue(*it, *iit);

                            ring_type current_output;
                            geometry::append(current_output, it->point);

                            turn_iterator current = it;
                            turn_operation_iterator_type current_iit = iit;
                            segment_identifier current_seg_id;

                            if (! detail::overlay::assign_next_ip<Reverse1, Reverse2>(
                                        geometry1, geometry2,
                                        turns,
                                        current, current_output,
                                        *iit, current_seg_id))
                            {
                                Backtrack::apply(
                                    size_at_start, 
                                    rings, current_output, turns, *current_iit,
                                    "No next IP",
                                    geometry1, geometry2, state);
                            }

                            if (! detail::overlay::select_next_ip(
                                            operation,
                                            *current,
                                            current_seg_id,
                                            current_iit))
                            {
                                Backtrack::apply(
                                    size_at_start, 
                                    rings, current_output, turns, *iit,
                                    "Dead end at start",
                                    geometry1, geometry2, state);
                            }
                            else
                            {

                                iit->visited.set_started();
                                detail::overlay::debug_traverse(*it, *iit, "-> Started");
                                detail::overlay::debug_traverse(*current, *current_iit, "Selected  ");


                                unsigned int i = 0;

                                while (current_iit != iit && state.good())
                                {
                                    if (current_iit->visited.visited())
                                    {
                                        // It visits a visited node again, without passing the start node.
                                        // This makes it suspicious for endless loops
                                        Backtrack::apply(
                                            size_at_start, 
                                            rings,  current_output, turns, *iit,
                                            "Visit again",
                                            geometry1, geometry2, state);
                                    }
                                    else
                                    {


                                        // We assume clockwise polygons only, non self-intersecting, closed.
                                        // However, the input might be different, and checking validity
                                        // is up to the library user.

                                        // Therefore we make here some sanity checks. If the input
                                        // violates the assumptions, the output polygon will not be correct
                                        // but the routine will stop and output the current polygon, and
                                        // will continue with the next one.

                                        // Below three reasons to stop.
                                        detail::overlay::assign_next_ip<Reverse1, Reverse2>(
                                            geometry1, geometry2,
                                            turns, current, current_output,
                                            *current_iit, current_seg_id);

                                        if (! detail::overlay::select_next_ip(
                                                    operation,
                                                    *current,
                                                    current_seg_id,
                                                    current_iit))
                                        {
                                            // Should not occur in valid (non-self-intersecting) polygons
                                            // Should not occur in self-intersecting polygons without spikes
                                            // Might occur in polygons with spikes
                                            Backtrack::apply(
                                                size_at_start, 
                                                rings,  current_output, turns, *iit,
                                                "Dead end",
                                                geometry1, geometry2, state);
                                        }
                                        else
                                        {
                                            detail::overlay::debug_traverse(*current, *current_iit, "Selected  ");
                                        }

                                        if (i++ > 2 + 2 * turns.size())
                                        {
                                            // Sanity check: there may be never more loops
                                            // than turn points.
                                            // Turn points marked as "ii" can be visited twice.
                                            Backtrack::apply(
                                                size_at_start, 
                                                rings,  current_output, turns, *iit,
                                                "Endless loop",
                                                geometry1, geometry2, state);
                                        }
                                    }
                                }

                                if (state.good())
                                {
                                    iit->visited.set_finished();
                                    detail::overlay::debug_traverse(*current, *iit, "->Finished");
                                    if (geometry::num_points(current_output) >= min_num_points)
                                    {
                                        rings.push_back(current_output);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } while (! state.good());
    }
};

}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_TRAVERSE_HPP
