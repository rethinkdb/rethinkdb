// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_HANDLE_TANGENCIES_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_HANDLE_TANGENCIES_HPP

#include <algorithm>

#include <boost/geometry/algorithms/detail/ring_identifier.hpp>
#include <boost/geometry/algorithms/detail/overlay/copy_segment_point.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>

#include <boost/geometry/geometries/segment.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


template
<
    typename TurnPoints,
    typename Indexed,
    typename Geometry1, typename Geometry2,
    bool Reverse1, bool Reverse2,
    typename Strategy
>
struct sort_in_cluster
{
    inline sort_in_cluster(TurnPoints const& turn_points
            , Geometry1 const& geometry1
            , Geometry2 const& geometry2
            , Strategy const& strategy)
        : m_turn_points(turn_points)
        , m_geometry1(geometry1)
        , m_geometry2(geometry2)
        , m_strategy(strategy)
    {}

private :

    TurnPoints const& m_turn_points;
    Geometry1 const& m_geometry1;
    Geometry2 const& m_geometry2;
    Strategy const& m_strategy;

    typedef typename Indexed::type turn_operation_type;
    typedef typename geometry::point_type<Geometry1>::type point_type;
    typedef model::referring_segment<point_type const> segment_type;

    // Determine how p/r and p/s are located.
    template <typename P>
    static inline void overlap_info(P const& pi, P const& pj,
        P const& ri, P const& rj,
        P const& si, P const& sj,
        bool& pr_overlap, bool& ps_overlap, bool& rs_overlap)
    {
        // Determine how p/r and p/s are located.
        // One of them is coming from opposite direction.

        typedef strategy::intersection::relate_cartesian_segments
            <
                policies::relate::segments_intersection_points
                    <
                        segment_type,
                        segment_type,
                        segment_intersection_points<point_type>
                    >
            > policy;

        segment_type p(pi, pj);
        segment_type r(ri, rj);
        segment_type s(si, sj);

        // Get the intersection point (or two points)
        segment_intersection_points<point_type> pr = policy::apply(p, r);
        segment_intersection_points<point_type> ps = policy::apply(p, s);
        segment_intersection_points<point_type> rs = policy::apply(r, s);

        // Check on overlap
        pr_overlap = pr.count == 2;
        ps_overlap = ps.count == 2;
        rs_overlap = rs.count == 2;
    }


#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
    inline void debug_consider(int order, Indexed const& left,
            Indexed const& right, std::string const& header,
            bool skip = true,
            std::string const& extra = "", bool ret = false
        ) const
    {
        if (skip) return;

        point_type pi, pj, ri, rj, si, sj;
        geometry::copy_segment_points<Reverse1, Reverse2>(m_geometry1, m_geometry2,
            left.subject.seg_id,
            pi, pj);
        geometry::copy_segment_points<Reverse1, Reverse2>(m_geometry1, m_geometry2,
            left.subject.other_id,
            ri, rj);
        geometry::copy_segment_points<Reverse1, Reverse2>(m_geometry1, m_geometry2,
            right.subject.other_id,
            si, sj);

        bool prc = false, psc = false, rsc = false;
        overlap_info(pi, pj, ri, rj, si, sj, prc, psc, rsc);

        int const side_ri_p = m_strategy.apply(pi, pj, ri);
        int const side_rj_p = m_strategy.apply(pi, pj, rj);
        int const side_si_p = m_strategy.apply(pi, pj, si);
        int const side_sj_p = m_strategy.apply(pi, pj, sj);
        int const side_si_r = m_strategy.apply(ri, rj, si);
        int const side_sj_r = m_strategy.apply(ri, rj, sj);

        std::cout << "Case: " << header << " for " << left.index << " / " << right.index << std::endl;
#ifdef BOOST_GEOMETRY_DEBUG_ENRICH_MORE
        std::cout << " Segment p:" << geometry::wkt(pi) << " .. " << geometry::wkt(pj) << std::endl;
        std::cout << " Segment r:" << geometry::wkt(ri) << " .. " << geometry::wkt(rj) << std::endl;
        std::cout << " Segment s:" << geometry::wkt(si) << " .. " << geometry::wkt(sj) << std::endl;

        std::cout << " r//p: " << side_ri_p << " / " << side_rj_p << std::endl;
        std::cout << " s//p: " << side_si_p << " / " << side_sj_p << std::endl;
        std::cout << " s//r: " << side_si_r << " / " << side_sj_r << std::endl;
#endif

        std::cout << header
                //<< " order: " << order
                << " ops: " << operation_char(left.subject.operation)
                    << "/" << operation_char(right.subject.operation)
                << " ri//p: " << side_ri_p
                << " si//p: " << side_si_p
                << " si//r: " << side_si_r
                << " cnts: " << int(prc) << ","  << int(psc) << "," << int(rsc)
                //<< " idx: " << left.index << "/" << right.index
                ;

        if (! extra.empty())
        {
            std::cout << " " << extra << " " << (ret ? "true" : "false");
        }
        std::cout << std::endl;
    }
#else
    inline void debug_consider(int, Indexed const& ,
            Indexed const& , std::string const& ,
            bool = true,
            std::string const& = "", bool = false
        ) const
    {}
#endif


    // ux/ux
    inline bool consider_ux_ux(Indexed const& left,
            Indexed const& right
            , std::string const& // header
        ) const
    {
        bool ret = left.index < right.index;

        // In combination of u/x, x/u: take first union, then blocked.
        // Solves #88, #61, #56, #80
        if (left.subject.operation == operation_union
            && right.subject.operation == operation_blocked)
        {
            ret = true;
        }
        else if (left.subject.operation == operation_blocked
            && right.subject.operation == operation_union)
        {
            ret = false;
        }
        else
        {
#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
            std::cout << "ux/ux unhandled" << std::endl;
#endif
        }

        //debug_consider(0, left, right, header, false, "-> return ", ret);

        return ret;
    }

    inline bool consider_iu_ux(Indexed const& left,
            Indexed const& right,
            int order // 1: iu first, -1: ux first
            , std::string const& // header
        ) const
    {
        bool ret = false;

        if (left.subject.operation == operation_union
            && right.subject.operation == operation_union)
        {
            ret = order == 1;
        }
        else if (left.subject.operation == operation_union
            && right.subject.operation == operation_blocked)
        {
            ret = true;
        }
        else if (right.subject.operation == operation_union
            && left.subject.operation == operation_blocked)
        {
            ret = false;
        }
        else if (left.subject.operation == operation_union)
        {
            ret = true;
        }
        else if (right.subject.operation == operation_union)
        {
            ret = false;
        }
        else
        {
#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
            // this still happens in the traverse.cpp test
            std::cout << " iu/ux unhandled" << std::endl;
#endif
            ret = order == 1;
        }

        //debug_consider(0, left, right, header, false, "-> return", ret);
        return ret;
    }

    inline bool consider_iu_ix(Indexed const& left,
            Indexed const& right,
            int order // 1: iu first, -1: ix first
            , std::string const& // header
        ) const
    {
        //debug_consider(order, left, right, header, false, "iu/ix");

        return left.subject.operation == operation_intersection
                && right.subject.operation == operation_intersection ? order == 1
            : left.subject.operation == operation_intersection ? false
            : right.subject.operation == operation_intersection ? true
            : order == 1;
    }

    inline bool consider_ix_ix(Indexed const& left, Indexed const& right
            , std::string const& // header
            ) const
    {
        // Take first intersection, then blocked.
        if (left.subject.operation == operation_intersection
            && right.subject.operation == operation_blocked)
        {
            return true;
        }
        else if (left.subject.operation == operation_blocked
            && right.subject.operation == operation_intersection)
        {
            return false;
        }

        // Default case, should not occur

#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
        std::cout << "ix/ix unhandled" << std::endl;
#endif
        //debug_consider(0, left, right, header, false, "-> return", ret);

        return left.index < right.index;
    }


    inline bool consider_iu_iu(Indexed const& left, Indexed const& right,
                    std::string const& header) const
    {
        //debug_consider(0, left, right, header);

        // In general, order it like "union, intersection".
        if (left.subject.operation == operation_intersection
            && right.subject.operation == operation_union)
        {
            //debug_consider(0, left, right, header, false, "i,u", false);
            return false;
        }
        else if (left.subject.operation == operation_union
            && right.subject.operation == operation_intersection)
        {
            //debug_consider(0, left, right, header, false, "u,i", true);
            return true;
        }

        point_type pi, pj, ri, rj, si, sj;
        geometry::copy_segment_points<Reverse1, Reverse2>(m_geometry1, m_geometry2,
            left.subject.seg_id,
            pi, pj);
        geometry::copy_segment_points<Reverse1, Reverse2>(m_geometry1, m_geometry2,
            left.subject.other_id,
            ri, rj);
        geometry::copy_segment_points<Reverse1, Reverse2>(m_geometry1, m_geometry2,
            right.subject.other_id,
            si, sj);

        int const side_ri_p = m_strategy.apply(pi, pj, ri);
        int const side_si_p = m_strategy.apply(pi, pj, si);
        int const side_si_r = m_strategy.apply(ri, rj, si);

        // Both located at same side (#58, pie_21_7_21_0_3)
        if (side_ri_p * side_si_p == 1 && side_si_r != 0)
        {
            // Take the most left one
            if (left.subject.operation == operation_union
                && right.subject.operation == operation_union)
            {
                bool ret = side_si_r == 1;
                //debug_consider(0, left, right, header, false, "same side", ret);
                return ret;
            }
        }


        // Coming from opposite sides (#59, #99)
        if (side_ri_p * side_si_p == -1)
        {
            bool ret = false;

            {
                ret = side_ri_p == 1; // #100
                debug_consider(0, left, right, header, false, "opp.", ret);
                return ret;
            }
#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
            std::cout << " iu/iu coming from opposite unhandled" << std::endl;
#endif
        }

        // We need EXTRA information here: are p/r/s overlapping?
        bool pr_ov = false, ps_ov = false, rs_ov = false;
        overlap_info(pi, pj, ri, rj, si, sj, pr_ov, ps_ov, rs_ov);

        // One coming from right (#83,#90)
        // One coming from left (#90, #94, #95)
        if (side_si_r != 0 && (side_ri_p != 0 || side_si_p != 0))
        {
            bool ret = false;

            if (pr_ov || ps_ov)
            {
                int r = side_ri_p != 0 ? side_ri_p : side_si_p;
                ret = r * side_si_r == 1;
            }
            else
            {
                ret = side_si_r == 1;
            }

            debug_consider(0, left, right, header, false, "left or right", ret);
            return ret;
        }

        // All aligned (#92, #96)
        if (side_ri_p == 0 && side_si_p == 0 && side_si_r == 0)
        {
            // One of them is coming from opposite direction.

            // Take the one NOT overlapping
            bool ret = false;
            bool found = false;
            if (pr_ov && ! ps_ov)
            {
                ret = true;
                found = true;
            }
            else if (!pr_ov && ps_ov)
            {
                ret = false;
                found = true;
            }

            debug_consider(0, left, right, header, false, "aligned", ret);
            if (found)
            {
                return ret;
            }
        }

#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
        std::cout << " iu/iu unhandled" << std::endl;
        debug_consider(0, left, right, header, false, "unhandled", left.index < right.index);
#endif
        return left.index < right.index;
    }

    inline bool consider_ii(Indexed const& left, Indexed const& right,
                    std::string const& header) const
    {
        debug_consider(0, left, right, header);

        point_type pi, pj, ri, rj, si, sj;
        geometry::copy_segment_points<Reverse1, Reverse2>(m_geometry1, m_geometry2,
            left.subject.seg_id,
            pi, pj);
        geometry::copy_segment_points<Reverse1, Reverse2>(m_geometry1, m_geometry2,
            left.subject.other_id,
            ri, rj);
        geometry::copy_segment_points<Reverse1, Reverse2>(m_geometry1, m_geometry2,
            right.subject.other_id,
            si, sj);

        int const side_ri_p = m_strategy.apply(pi, pj, ri);
        int const side_si_p = m_strategy.apply(pi, pj, si);

        // Two other points are (mostly) lying both right of the considered segment
        // Take the most left one
        int const side_si_r = m_strategy.apply(ri, rj, si);
        if (side_ri_p == -1
            && side_si_p == -1
            && side_si_r != 0)
        {
            bool const ret = side_si_r != 1;
            return ret;
        }
        return left.index < right.index;
    }


public :
    inline bool operator()(Indexed const& left, Indexed const& right) const
    {
        bool const default_order = left.index < right.index;

        if ((m_turn_points[left.index].discarded || left.discarded)
            && (m_turn_points[right.index].discarded || right.discarded))
        {
            return default_order;
        }
        else if (m_turn_points[left.index].discarded || left.discarded)
        {
            // Be careful to sort discarded first, then all others
            return true;
        }
        else if (m_turn_points[right.index].discarded || right.discarded)
        {
            // See above so return false here such that right (discarded)
            // is sorted before left (not discarded)
            return false;
        }
        else if (m_turn_points[left.index].combination(operation_blocked, operation_union)
                && m_turn_points[right.index].combination(operation_blocked, operation_union))
        {
            // ux/ux
            return consider_ux_ux(left, right, "ux/ux");
        }
        else if (m_turn_points[left.index].both(operation_union)
            && m_turn_points[right.index].both(operation_union))
        {
            // uu/uu, Order is arbitrary
            // Note: uu/uu is discarded now before so this point will
            //       not be reached.
            return default_order;
        }
        else if (m_turn_points[left.index].combination(operation_intersection, operation_union)
                && m_turn_points[right.index].combination(operation_intersection, operation_union))
        {
            return consider_iu_iu(left, right, "iu/iu");
        }
        else if (m_turn_points[left.index].combination(operation_intersection, operation_blocked)
                && m_turn_points[right.index].combination(operation_intersection, operation_blocked))
        {
            return consider_ix_ix(left, right, "ix/ix");
        }
        else if (m_turn_points[left.index].both(operation_intersection)
                && m_turn_points[right.index].both(operation_intersection))
        {
            return consider_ii(left, right, "ii/ii");
        }
        else  if (m_turn_points[left.index].combination(operation_union, operation_blocked)
                && m_turn_points[right.index].combination(operation_intersection, operation_union))
        {
            return consider_iu_ux(left, right, -1, "ux/iu");
        }
        else if (m_turn_points[left.index].combination(operation_intersection, operation_union)
                && m_turn_points[right.index].combination(operation_union, operation_blocked))
        {
            return consider_iu_ux(left, right, 1, "iu/ux");
        }
        else  if (m_turn_points[left.index].combination(operation_intersection, operation_blocked)
                && m_turn_points[right.index].combination(operation_intersection, operation_union))
        {
            return consider_iu_ix(left, right, 1, "ix/iu");
        }
        else if (m_turn_points[left.index].combination(operation_intersection, operation_union)
                && m_turn_points[right.index].combination(operation_intersection, operation_blocked))
        {
            return consider_iu_ix(left, right, -1, "iu/ix");
        }
        else if (m_turn_points[left.index].method != method_equal
            && m_turn_points[right.index].method == method_equal
            )
        {
            // If one of them was EQUAL or CONTINUES, it should always come first
            return false;
        }
        else if (m_turn_points[left.index].method == method_equal
            && m_turn_points[right.index].method != method_equal
            )
        {
            return true;
        }

        // Now we have no clue how to sort.

#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
        std::cout << " Consider: " << operation_char(m_turn_points[left.index].operations[0].operation)
                << operation_char(m_turn_points[left.index].operations[1].operation)
                << "/" << operation_char(m_turn_points[right.index].operations[0].operation)
                << operation_char(m_turn_points[right.index].operations[1].operation)
                << " " << " Take " << left.index << " < " << right.index
                << std::endl;
#endif

        return default_order;
    }
};



template
<
    typename IndexType,
    typename Iterator,
    typename TurnPoints,
    typename Geometry1,
    typename Geometry2,
    typename Strategy
>
inline void inspect_cluster(Iterator begin_cluster, Iterator end_cluster,
            TurnPoints& turn_points,
            operation_type ,
            Geometry1 const& , Geometry2 const& ,
            Strategy const& )
{
    int count = 0;

    // Make an analysis about all occuring cases here.
    std::map<std::pair<operation_type, operation_type>, int> inspection;
    for (Iterator it = begin_cluster; it != end_cluster; ++it)
    {
        operation_type first = turn_points[it->index].operations[0].operation;
        operation_type second = turn_points[it->index].operations[1].operation;
        if (first > second)
        {
            std::swap(first, second);
        }
        inspection[std::make_pair(first, second)]++;
        count++;
    }


    bool keep_cc = false;

    // Decide about which is going to be discarded here.
    if (inspection[std::make_pair(operation_union, operation_union)] == 1
        && inspection[std::make_pair(operation_continue, operation_continue)] == 1)
    {
        // In case of uu/cc, discard the uu, that indicates a tangency and
        // inclusion would disturb the (e.g.) cc-cc-cc ordering
        // NOTE: uu is now discarded anyhow.
        keep_cc = true;
    }
    else if (count == 2
        && inspection[std::make_pair(operation_intersection, operation_intersection)] == 1
        && inspection[std::make_pair(operation_union, operation_intersection)] == 1)
    {
        // In case of ii/iu, discard the iu. The ii should always be visited,
        // Because (in case of not discarding iu) correctly ordering of ii/iu appears impossible
        for (Iterator it = begin_cluster; it != end_cluster; ++it)
        {
            if (turn_points[it->index].combination(operation_intersection, operation_union))
            {
                it->discarded = true;
            }
        }
    }

    // Discard any continue turn, unless it is the only thing left
    //    (necessary to avoid cc-only rings, all being discarded
    //    e.g. traversal case #75)
    int nd_count= 0, cc_count = 0;
    for (Iterator it = begin_cluster; it != end_cluster; ++it)
    {
        if (! it->discarded)
        {
            nd_count++;
            if (turn_points[it->index].both(operation_continue))
            {
                cc_count++;
            }
        }
    }

    if (nd_count == cc_count)
    {
        keep_cc = true;
    }

    if (! keep_cc)
    {
        for (Iterator it = begin_cluster; it != end_cluster; ++it)
        {
            if (turn_points[it->index].both(operation_continue))
            {
                it->discarded = true;
            }
        }
    }
}


template
<
    typename IndexType,
    bool Reverse1, bool Reverse2,
    typename Iterator,
    typename TurnPoints,
    typename Geometry1,
    typename Geometry2,
    typename Strategy
>
inline void handle_cluster(Iterator begin_cluster, Iterator end_cluster,
            TurnPoints& turn_points,
            operation_type for_operation,
            Geometry1 const& geometry1, Geometry2 const& geometry2,
            Strategy const& strategy)
{
    // First inspect and (possibly) discard rows
    inspect_cluster<IndexType>(begin_cluster, end_cluster, turn_points,
            for_operation, geometry1, geometry2, strategy);


    // Then sort this range (discard rows will be ordered first and will be removed in enrich_assign)
    std::sort(begin_cluster, end_cluster,
                sort_in_cluster
                    <
                        TurnPoints,
                        IndexType,
                        Geometry1, Geometry2,
                        Reverse1, Reverse2,
                        Strategy
                    >(turn_points, geometry1, geometry2, strategy));


#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
    typedef typename IndexType::type operations_type;
    operations_type const& op = turn_points[begin_cluster->index].operations[begin_cluster->operation_index];
    std::cout << "Clustered points on equal distance " << op.enriched.distance << std::endl;
    std::cout << "->Indexes ";

    for (Iterator it = begin_cluster; it != end_cluster; ++it)
    {
        std::cout << " " << it->index;
    }
    std::cout << std::endl << "->Methods: ";
    for (Iterator it = begin_cluster; it != end_cluster; ++it)
    {
        std::cout << " " << method_char(turn_points[it->index].method);
    }
    std::cout << std::endl << "->Operations: ";
    for (Iterator it = begin_cluster; it != end_cluster; ++it)
    {
        std::cout << " " << operation_char(turn_points[it->index].operations[0].operation)
            << operation_char(turn_points[it->index].operations[1].operation);
    }
    std::cout << std::endl << "->Discarded: ";
    for (Iterator it = begin_cluster; it != end_cluster; ++it)
    {
        std::cout << " " << (it->discarded ? "true" : "false");
    }
    std::cout << std::endl;
        //<< "\tOn segments: "    << prev_op.seg_id  << " / "  << prev_op.other_id
        //<< " and "  << op.seg_id << " / " << op.other_id
        //<< geometry::distance(turn_points[prev->index].point, turn_points[it->index].point)
#endif

}


}} // namespace detail::overlay
#endif //DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_HANDLE_TANGENCIES_HPP
