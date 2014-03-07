// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_GET_LEFT_TURNS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_GET_LEFT_TURNS_HPP

#include <boost/geometry/iterators/ever_circling_iterator.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

// TODO: move this to /util/
template <typename T>
static inline std::pair<T, T> ordered_pair(T const& first, T const& second)
{
    return first < second ? std::make_pair(first, second) : std::make_pair(second, first);
}

template <typename AngleInfo>
inline void debug_left_turn(AngleInfo const& ai, AngleInfo const& previous)
{
#ifdef BOOST_GEOMETRY_DEBUG_BUFFER_OCCUPATION
    std::cout << "Angle: " << (ai.incoming ? "i" : "o") 
        << " " << si(ai.seg_id) 
        << " " << (math::r2d * (ai.angle) )
        << " turn: " << ai.turn_index << "[" << ai.operation_index << "]"
        ;

    if (ai.turn_index != previous.turn_index
        || ai.operation_index != previous.operation_index)
    {
        std::cout << " diff: " << math::r2d * math::abs(previous.angle - ai.angle);
    }
    std::cout << std::endl;
#endif
}

template <typename AngleInfo>
inline void debug_left_turn(std::string const& caption, AngleInfo const& ai, AngleInfo const& previous,
            int code = -99, int code2 = -99, int code3 = -99, int code4 = -99)
{
#ifdef BOOST_GEOMETRY_DEBUG_BUFFER_OCCUPATION
    std::cout << " " << caption
            << " turn: " << ai.turn_index << "[" << ai.operation_index << "]"
            << " " << si(ai.seg_id) 
            << " " << (ai.incoming ? "i" : "o") 
            << " " << (math::r2d * (ai.angle) )
            << " turn: " << previous.turn_index << "[" << previous.operation_index << "]"
            << " " << si(previous.seg_id) 
            << " " << (previous.incoming ? "i" : "o") 
            << " " << (math::r2d * (previous.angle) )
            ;

    if (code != -99)
    {
        std::cout << " code: " << code << " , " << code2 << " , " << code3 << " , " << code4;
    }
    std::cout << std::endl;
#endif
}


template <typename Operation>
inline bool include_operation(Operation const& op, 
                segment_identifier const& outgoing_seg_id,
                segment_identifier const& incoming_seg_id)
{
    return op.seg_id == outgoing_seg_id
        && op.other_id == incoming_seg_id
        && (op.operation == detail::overlay::operation_union
            ||op.operation == detail::overlay::operation_continue)
            ;
}

template <typename Turn>
inline bool process_include(segment_identifier const& outgoing_seg_id, segment_identifier const& incoming_seg_id,
                int turn_index, Turn& turn,
                std::set<int>& keep_indices, int priority)
{
    bool result = false;
    for (int i = 0; i < 2; i++)
    {
        if (include_operation(turn.operations[i], outgoing_seg_id, incoming_seg_id))
        {
            turn.operations[i].include_in_occupation_map = true;
            if (priority > turn.priority)
            {
                turn.priority = priority;
            }
            keep_indices.insert(turn_index);
            result = true;
        }
    }
    return result;
}

template <typename AngleInfo, typename Turns, typename TurnSegmentIndices>
inline bool include_left_turn_of_all(
                AngleInfo const& outgoing, AngleInfo const& incoming,
                Turns& turns, TurnSegmentIndices const& turn_segment_indices,
                std::set<int>& keep_indices, int priority)
{
    segment_identifier const& outgoing_seg_id = turns[outgoing.turn_index].operations[outgoing.operation_index].seg_id;
    segment_identifier const& incoming_seg_id = turns[incoming.turn_index].operations[incoming.operation_index].seg_id;

    if (outgoing.turn_index == incoming.turn_index)
    {
        return process_include(outgoing_seg_id, incoming_seg_id, outgoing.turn_index, turns[outgoing.turn_index], keep_indices, priority);
    }

    bool result = false;
    std::pair<segment_identifier, segment_identifier> pair = ordered_pair(outgoing_seg_id, incoming_seg_id);
    typename boost::range_iterator<TurnSegmentIndices const>::type it = turn_segment_indices.find(pair);
    if (it != turn_segment_indices.end())
    {
        for (std::set<int>::const_iterator sit = it->second.begin(); sit != it->second.end(); ++sit)
        {
            if (process_include(outgoing_seg_id, incoming_seg_id, *sit, turns[*sit], keep_indices, priority))
            {
                result = true;
            }
        }
    }
    return result;
}

template <std::size_t Index, typename Turn>
inline bool corresponds(Turn const& turn, segment_identifier const& seg_id)
{
    return turn.operations[Index].operation == detail::overlay::operation_union
        && turn.operations[Index].seg_id == seg_id;
}


template <typename Turns, typename TurnSegmentIndices>
inline bool prefer_by_other(Turns const& turns,
            TurnSegmentIndices const& turn_segment_indices,
            std::set<int>& indices)
{
    std::map<segment_identifier, int> map;
    for (std::set<int>::const_iterator sit = indices.begin();
        sit != indices.end();
        ++sit)
    {
        map[turns[*sit].operations[0].seg_id]++;
        map[turns[*sit].operations[1].seg_id]++;
    }

    std::set<segment_identifier> segment_occuring_once;
    for (std::map<segment_identifier, int>::const_iterator mit = map.begin(); 
        mit != map.end();++mit)
    {
        if (mit->second == 1)
        {
            segment_occuring_once.insert(mit->first);
        }
#ifdef BOOST_GEOMETRY_DEBUG_BUFFER_PREFER
        std::cout << si(mit->first) << " " << mit->second << std::endl;
#endif
    }

    if (segment_occuring_once.size() == 2)
    {
        // Try to find within all turns a turn with these two segments

        std::set<segment_identifier>::const_iterator soo_it = segment_occuring_once.begin();
        segment_identifier front = *soo_it;
        soo_it++;
        segment_identifier back = *soo_it;

        std::pair<segment_identifier, segment_identifier> pair = ordered_pair(front, back);

        typename boost::range_iterator<TurnSegmentIndices const>::type it = turn_segment_indices.find(pair);
        if (it != turn_segment_indices.end())
        {
            // debug_turns_by_indices("Found", it->second);
            // Check which is the union/continue
            segment_identifier good;
            for (std::set<int>::const_iterator sit = it->second.begin(); sit != it->second.end(); ++sit)
            {
                if (turns[*sit].operations[0].operation == detail::overlay::operation_union)
                {
                    good = turns[*sit].operations[0].seg_id;
                }
                else if (turns[*sit].operations[1].operation == detail::overlay::operation_union)
                {
                    good = turns[*sit].operations[1].seg_id;
                }
            }

#ifdef BOOST_GEOMETRY_DEBUG_BUFFER_PREFER
            std::cout << "Good: " << si(good) << std::endl;
#endif

            // Find in indexes-to-keep this segment with the union. Discard the other one
            std::set<int> ok_indices;
            for (std::set<int>::const_iterator sit = indices.begin(); sit != indices.end(); ++sit)
            {
                if (corresponds<0>(turns[*sit], good) || corresponds<1>(turns[*sit], good))
                {
                    ok_indices.insert(*sit);
                }
            }
            if (ok_indices.size() > 0 && ok_indices.size() < indices.size())
            {
                indices = ok_indices;
                std::cout << "^";
                return true;
            }
        }
    }
    return false;
}

template <typename Turns>
inline void prefer_by_priority(Turns const& turns, std::set<int>& indices)
{
    // Find max prio
    int min_prio = 1024, max_prio = 0;
    for (std::set<int>::const_iterator sit = indices.begin(); sit != indices.end(); ++sit)
    {
        if (turns[*sit].priority > max_prio)
        {
            max_prio = turns[*sit].priority;
        }
        if (turns[*sit].priority < min_prio)
        {
            min_prio = turns[*sit].priority;
        }
    }

    if (min_prio == max_prio)
    {
        return;
    }

    // Only keep indices with high prio
    std::set<int> ok_indices;
    for (std::set<int>::const_iterator sit = indices.begin(); sit != indices.end(); ++sit)
    {
        if (turns[*sit].priority >= max_prio)
        {
            ok_indices.insert(*sit);
        }
    }
    if (ok_indices.size() > 0 && ok_indices.size() < indices.size())
    {
        indices = ok_indices;
        std::cout << "%";
    }
}

template <typename AngleInfo, typename Angles, typename Turns, typename TurnSegmentIndices>
inline void calculate_left_turns(Angles const& angles, 
                Turns& turns, TurnSegmentIndices const& turn_segment_indices,
                std::set<int>& keep_indices)
{
    bool debug_indicate_size = false;

    typedef typename strategy::side::services::default_strategy
        <
            typename cs_tag<typename AngleInfo::point_type>::type
        >::type side_strategy;

    std::size_t i = 0;
    std::size_t n = boost::size(angles);

    typedef geometry::ever_circling_range_iterator<Angles const> circling_iterator;
    circling_iterator cit(angles);

    debug_left_turn(*cit, *cit);
    for(circling_iterator prev = cit++; i < n; prev = cit++, i++)
    {
        debug_left_turn(*cit, *prev);

        bool const include = ! geometry::math::equals(prev->angle, cit->angle)
            && ! prev->incoming
            && cit->incoming;

        if (include)
        {
            // Go back to possibly include more same left outgoing angles:
            // Because we check on side too we can take a large "epsilon"
            circling_iterator back = prev - 1;

            typename AngleInfo::angle_type eps = 0.00001;
            int b = 1;
            for(std::size_t d = 0; 
                    math::abs(prev->angle - back->angle) < eps 
                        && ! back->incoming 
                        && d < n;
                d++)
            {
                --back;
                ++b;
            }

            // Same but forward to possibly include more incoming angles
            int f = 1;
            circling_iterator forward = cit + 1;
            for(std::size_t d = 0;
                    math::abs(cit->angle - forward->angle) < eps 
                        && forward->incoming 
                        && d < n;
                    d++)
            {
                ++forward;
                ++f;
            }

#ifdef BOOST_GEOMETRY_DEBUG_BUFFER_OCCUPATION
            std::cout << "HANDLE " << b << "/" << f << " ANGLES" << std::endl;
#endif
            for(circling_iterator bit = prev; bit != back; --bit)
            {
                int code = side_strategy::apply(bit->direction_point, prev->intersection_point, prev->direction_point);
                int code2 = side_strategy::apply(prev->direction_point, bit->intersection_point, bit->direction_point);
                for(circling_iterator fit = cit; fit != forward; ++fit)
                {
                    int code3 = side_strategy::apply(fit->direction_point, cit->intersection_point, cit->direction_point);
                    int code4 = side_strategy::apply(cit->direction_point, fit->intersection_point, fit->direction_point);

                    int priority = 2;
                    if (code == -1 && code2 == 1)
                    {
                        // This segment is lying right of the other one.
                        // Cannot ignore it (because of robustness, see a.o. #rt_p21 from unit test).
                        // But if we find more we can prefer later by priority
                        // (a.o. necessary for #rt_o2 from unit test)
                        priority = 1;
                    }

                    bool included = include_left_turn_of_all(*bit, *fit, turns, turn_segment_indices, keep_indices, priority);
                    debug_left_turn(included ? "KEEP" : "SKIP", *fit, *bit, code, code2, code3, code4);
                }
            }
        }
    }

    if (debug_indicate_size)
    {
        std::cout << " size=" << keep_indices.size();
    }

    if (keep_indices.size() >= 2)
    {
        prefer_by_other(turns, turn_segment_indices, keep_indices);
    }
    if (keep_indices.size() >= 2)
    {
            prefer_by_priority(turns, keep_indices);
    }
}

} // namespace detail
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_GET_LEFT_TURNS_HPP
