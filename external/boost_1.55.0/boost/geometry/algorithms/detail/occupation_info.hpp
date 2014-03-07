// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OCCUPATION_INFO_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OCCUPATION_INFO_HPP

#if ! defined(NDEBUG)
//  #define BOOST_GEOMETRY_DEBUG_BUFFER_OCCUPATION
#endif

#include <algorithm>
#include <boost/range.hpp>

#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/point_type.hpp>

#include <boost/geometry/algorithms/equals.hpp>
#include <boost/geometry/iterators/closing_iterator.hpp>

#include <boost/geometry/algorithms/detail/get_left_turns.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename P>
class relaxed_less
{
    typedef typename geometry::coordinate_type<P>::type coordinate_type;

    coordinate_type epsilon;

public :

    inline relaxed_less()
    {
        // TODO: adapt for ttmath, and maybe build the map in another way 
        // (e.g. exact constellations of segment-id's), maybe adaptive.
        epsilon = std::numeric_limits<double>::epsilon() * 100.0;
    }

    inline bool operator()(P const& a, P const& b) const
    {
        coordinate_type const dx = math::abs(geometry::get<0>(a) - geometry::get<0>(b));
        coordinate_type const dy = math::abs(geometry::get<1>(a) - geometry::get<1>(b));


        if (dx < epsilon && dy < epsilon)
        {
            return false;
        }
        if (dx < epsilon)
        {
            return geometry::get<1>(a) < geometry::get<1>(b);
        }

        return geometry::get<0>(a) < geometry::get<0>(b);
    }

    inline bool equals(P const& a, P const& b) const
    {
        typedef typename geometry::coordinate_type<P>::type coordinate_type;

        coordinate_type const dx = math::abs(geometry::get<0>(a) - geometry::get<0>(b));
        coordinate_type const dy = math::abs(geometry::get<1>(a) - geometry::get<1>(b));

        return dx < epsilon && dy < epsilon;
    };
};


template <typename T, typename P1, typename P2>
inline T calculate_angle(P1 const& from_point, P2 const& to_point)
{
    typedef P1 vector_type;
    vector_type v = from_point;
    geometry::subtract_point(v, to_point);
    return atan2(geometry::get<1>(v), geometry::get<0>(v));
}

template <typename Iterator, typename Vector>
inline Iterator advance_circular(Iterator it, Vector const& vector, segment_identifier& seg_id, bool forward = true)
{
    int const increment = forward ? 1 : -1;
    if (it == boost::begin(vector) && increment < 0)
    {
        it = boost::end(vector);
        seg_id.segment_index = boost::size(vector);
    }
    it += increment;
    seg_id.segment_index += increment;
    if (it == boost::end(vector))
    {
        seg_id.segment_index = 0;
        it = boost::begin(vector);
    }
    return it;
}

template <typename Point, typename T>
struct angle_info
{
    typedef T angle_type;
    typedef Point point_type;

    segment_identifier seg_id;
    int turn_index;
    int operation_index;
    Point intersection_point;
    Point direction_point;
    T angle;
    bool incoming;
};

template <typename AngleInfo>
class occupation_info
{
    typedef std::vector<AngleInfo> collection_type;

    struct angle_sort
    {
        inline bool operator()(AngleInfo const& left, AngleInfo const& right) const
        {
            // In this case we can compare even double using equals
            // return geometry::math::equals(left.angle, right.angle)
            return left.angle == right.angle
                ? int(left.incoming) < int(right.incoming)
                : left.angle < right.angle
                ;
        }
    };

public :
    collection_type angles;
private :
    bool m_occupied;
    bool m_calculated;

    inline bool is_occupied()
    {
        if (boost::size(angles) <= 1)
        {
            return false;
        }

        std::sort(angles.begin(), angles.end(), angle_sort());

        typedef geometry::closing_iterator<collection_type const> closing_iterator;
        closing_iterator vit(angles);
        closing_iterator end(angles, true);

        closing_iterator prev = vit++;
        for( ; vit != end; prev = vit++)
        {
            if (! geometry::math::equals(prev->angle, vit->angle)
                && ! prev->incoming
                && vit->incoming)
            {
                return false;
            }
        }
        return true;
    }

public :
    inline occupation_info()
        : m_occupied(false)
        , m_calculated(false)
    {}

    template <typename PointC, typename Point1, typename Point2>
    inline void add(PointC const& map_point, Point1 const& direction_point, Point2 const& intersection_point,
                    int turn_index, int operation_index,
                    segment_identifier const& seg_id, bool incoming)
    {
        //std::cout << "-> adding angle " << geometry::wkt(direction_point) << " .. " << geometry::wkt(intersection_point) << " " << int(incoming) << std::endl;
        if (geometry::equals(direction_point, intersection_point))
        {
            //std::cout << "EQUAL! Skipping" << std::endl;
            return;
        }

        AngleInfo info;
        info.incoming = incoming;
        info.angle = calculate_angle<typename AngleInfo::angle_type>(direction_point, map_point);
        info.seg_id = seg_id;
        info.turn_index = turn_index;
        info.operation_index = operation_index;
        info.intersection_point = intersection_point;
        info.direction_point = direction_point;
        angles.push_back(info);

        m_calculated = false;
    }

    inline bool occupied()
    {
        if (! m_calculated)
        {
            m_occupied = is_occupied();
            m_calculated = true;
        }
        return m_occupied;
    }

    template <typename Turns, typename TurnSegmentIndices>
    inline void get_left_turns(
                    Turns& turns, TurnSegmentIndices const& turn_segment_indices,
                    std::set<int>& keep_indices)
    {
        std::sort(angles.begin(), angles.end(), angle_sort());
        calculate_left_turns<AngleInfo>(angles, turns, turn_segment_indices, keep_indices);
    }
};


template <typename Point, typename Ring, typename Info>
inline void add_incoming_and_outgoing_angles(Point const& map_point, Point const& intersection_point,
                Ring const& ring, 
                int turn_index,
                int operation_index,
                segment_identifier seg_id,
                Info& info)
{
    typedef typename boost::range_iterator
        <
            Ring const
        >::type iterator_type;

    int const n = boost::size(ring);
    if (seg_id.segment_index >= n || seg_id.segment_index < 0)
    {
        return;
    }

    segment_identifier real_seg_id = seg_id;
    iterator_type it = boost::begin(ring) + seg_id.segment_index;

    // TODO: if we use turn-info ("to", "middle"), we know if to advance without resorting to equals
    relaxed_less<Point> comparator;

    if (comparator.equals(intersection_point, *it))
    {
        // It should be equal only once. But otherwise we skip it (in "add")
        it = advance_circular(it, ring, seg_id, false);
    }

    info.add(map_point, *it, intersection_point, turn_index, operation_index, real_seg_id, true);

    if (comparator.equals(intersection_point, *it))
    {
        it = advance_circular(it, ring, real_seg_id);
    }
    else
    {
        // Don't upgrade the ID
        it = advance_circular(it, ring, seg_id);
    }
    for (int defensive_check = 0; 
        comparator.equals(intersection_point, *it) && defensive_check < n; 
        defensive_check++)
    {
        it = advance_circular(it, ring, real_seg_id);
    }

    info.add(map_point, *it, intersection_point, turn_index, operation_index, real_seg_id, false);
}


// Map in two senses of the word: it is a std::map where the key is a point.
// Per point an "occupation_info" record is kept
// Used for the buffer (but will also be used for intersections/unions having complex self-tangencies)
template <typename Point, typename OccupationInfo>
class occupation_map
{
public :
    typedef std::map<Point, OccupationInfo, relaxed_less<Point> > map_type;

    map_type map;
    std::set<int> turn_indices;

    inline OccupationInfo& find_or_insert(Point const& point, Point& mapped_point)
    {
        typename map_type::iterator it = map.find(point);
        if (it == boost::end(map))
        {
            std::pair<typename map_type::iterator, bool> pair 
                        = map.insert(std::make_pair(point, OccupationInfo()));
            it = pair.first;
        }
        mapped_point = it->first;
        return it->second;
    }

    inline bool contains(Point const& point) const
    {
        typename map_type::const_iterator it = map.find(point);
        return it != boost::end(map);
    }

    inline bool contains_turn_index(int index) const
    {
        return turn_indices.count(index) > 0;
    }

    inline void insert_turn_index(int index)
    {
        turn_indices.insert(index);
    }
};


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OCCUPATION_INFO_HPP
