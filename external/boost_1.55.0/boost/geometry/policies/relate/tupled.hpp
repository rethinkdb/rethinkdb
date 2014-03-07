// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_TUPLED_HPP
#define BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_TUPLED_HPP


#include <string>

#include <boost/tuple/tuple.hpp>
#include <boost/geometry/strategies/side_info.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>
#include <boost/geometry/util/select_most_precise.hpp>

namespace boost { namespace geometry
{

namespace policies { namespace relate
{


// "tupled" to return intersection results together.
// Now with two, with some meta-programming and derivations it can also be three (or more)
template <typename Policy1, typename Policy2, typename CalculationType = void>
struct segments_tupled
{
    typedef boost::tuple
        <
            typename Policy1::return_type,
            typename Policy2::return_type
        > return_type;

    // Take segments of first policy, they should be equal
    typedef typename Policy1::segment_type1 segment_type1;
    typedef typename Policy1::segment_type2 segment_type2;

    typedef typename select_calculation_type
        <
            segment_type1,
            segment_type2,
            CalculationType
        >::type coordinate_type;

    // Get the same type, but at least a double
    typedef typename select_most_precise<coordinate_type, double>::type rtype;

    template <typename R>
    static inline return_type segments_intersect(side_info const& sides,
                    R const& r,
                    coordinate_type const& dx1, coordinate_type const& dy1,
                    coordinate_type const& dx2, coordinate_type const& dy2,
                    segment_type1 const& s1, segment_type2 const& s2)
    {
        return boost::make_tuple
            (
                Policy1::segments_intersect(sides, r,
                    dx1, dy1, dx2, dy2, s1, s2),
                Policy2::segments_intersect(sides, r,
                    dx1, dy1, dx2, dy2, s1, s2)
            );
    }

    static inline return_type collinear_touch(coordinate_type const& x,
                coordinate_type const& y, int arrival_a, int arrival_b)
    {
        return boost::make_tuple
            (
                Policy1::collinear_touch(x, y, arrival_a, arrival_b),
                Policy2::collinear_touch(x, y, arrival_a, arrival_b)
            );
    }

    template <typename S>
    static inline return_type collinear_interior_boundary_intersect(S const& segment,
                bool a_within_b,
                int arrival_a, int arrival_b, bool opposite)
    {
        return boost::make_tuple
            (
                Policy1::collinear_interior_boundary_intersect(segment, a_within_b, arrival_a, arrival_b, opposite),
                Policy2::collinear_interior_boundary_intersect(segment, a_within_b, arrival_a, arrival_b, opposite)
            );
    }

    static inline return_type collinear_a_in_b(segment_type1 const& segment,
                bool opposite)
    {
        return boost::make_tuple
            (
                Policy1::collinear_a_in_b(segment, opposite),
                Policy2::collinear_a_in_b(segment, opposite)
            );
    }
    static inline return_type collinear_b_in_a(segment_type2 const& segment,
                    bool opposite)
    {
        return boost::make_tuple
            (
                Policy1::collinear_b_in_a(segment, opposite),
                Policy2::collinear_b_in_a(segment, opposite)
            );
    }


    static inline return_type collinear_overlaps(
                    coordinate_type const& x1, coordinate_type const& y1,
                    coordinate_type const& x2, coordinate_type const& y2,
                    int arrival_a, int arrival_b, bool opposite)
    {
        return boost::make_tuple
            (
                Policy1::collinear_overlaps(x1, y1, x2, y2, arrival_a, arrival_b, opposite),
                Policy2::collinear_overlaps(x1, y1, x2, y2, arrival_a, arrival_b, opposite)
            );
    }

    static inline return_type segment_equal(segment_type1 const& s,
                bool opposite)
    {
        return boost::make_tuple
            (
                Policy1::segment_equal(s, opposite),
                Policy2::segment_equal(s, opposite)
            );
    }

    static inline return_type degenerate(segment_type1 const& segment,
                bool a_degenerate)
    {
        return boost::make_tuple
            (
                Policy1::degenerate(segment, a_degenerate),
                Policy2::degenerate(segment, a_degenerate)
            );
    }

    static inline return_type disjoint()
    {
        return boost::make_tuple
            (
                Policy1::disjoint(),
                Policy2::disjoint()
            );
    }

    static inline return_type error(std::string const& msg)
    {
        return boost::make_tuple
            (
                Policy1::error(msg),
                Policy2::error(msg)
            );
    }

    static inline return_type collinear_disjoint()
    {
        return boost::make_tuple
            (
                Policy1::collinear_disjoint(),
                Policy2::collinear_disjoint()
            );
    }

};

}} // namespace policies::relate

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_TUPLED_HPP
