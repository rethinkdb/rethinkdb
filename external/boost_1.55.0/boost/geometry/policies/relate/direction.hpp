// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_DIRECTION_HPP
#define BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_DIRECTION_HPP


#include <cstddef>
#include <string>

#include <boost/concept_check.hpp>

#include <boost/geometry/arithmetic/determinant.hpp>
#include <boost/geometry/strategies/side_info.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>
#include <boost/geometry/util/select_most_precise.hpp>


namespace boost { namespace geometry
{


namespace policies { namespace relate
{

struct direction_type
{
    // NOTE: "char" will be replaced by enum in future version

    inline direction_type(side_info const& s, char h,
                int ha, int hb,
                int da = 0, int db = 0,
                bool op = false)
        : how(h)
        , opposite(op)
        , how_a(ha)
        , how_b(hb)
        , dir_a(da)
        , dir_b(db)
        , sides(s)
    {
        arrival[0] = ha;
        arrival[1] = hb;
    }

    inline direction_type(char h, bool op, int ha = 0, int hb = 0)
        : how(h)
        , opposite(op)
        , how_a(ha)
        , how_b(hb)
        , dir_a(0)
        , dir_b(0)
    {
        arrival[0] = ha;
        arrival[1] = hb;
    }


    // TODO: replace this
    // NOTE: "char" will be replaced by enum in future version
    // "How" is the intersection formed?
    char how;

    // Is it opposite (for collinear/equal cases)
    bool opposite;

    // Information on how A arrives at intersection, how B arrives at intersection
    // 1: arrives at intersection
    // -1: starts from intersection
    int how_a;
    int how_b;

    // Direction: how is A positioned from B
    // 1: points left, seen from IP
    // -1: points right, seen from IP
    // In case of intersection: B's TO direction
    // In case that B's TO direction is at A: B's from direction
    // In collinear cases: it is 0
    int dir_a; // Direction of A-s TO from IP
    int dir_b; // Direction of B-s TO from IP

    // New information
    side_info sides;
    int arrival[2]; // 1=arrival, -1departure, 0=neutral; == how_a//how_b


    // About arrival[0] (== arrival of a2 w.r.t. b) for COLLINEAR cases
    // Arrival  1: a1--------->a2         (a arrives within b)
    //                      b1----->b2

    // Arrival  1: (a in b)
    //


    // Arrival -1:      a1--------->a2     (a does not arrive within b)
    //             b1----->b2

    // Arrival -1: (b in a)               a_1-------------a_2
    //                                         b_1---b_2

    // Arrival  0:                        a1------->a2  (a arrives at TO-border of b)
    //                                        b1--->b2

};


template <typename S1, typename S2, typename CalculationType = void>
struct segments_direction
{
    typedef direction_type return_type;
    typedef S1 segment_type1;
    typedef S2 segment_type2;
    typedef typename select_calculation_type
        <
            S1, S2, CalculationType
        >::type coordinate_type;

    // Get the same type, but at least a double
    typedef typename select_most_precise<coordinate_type, double>::type rtype;


    template <typename R>
    static inline return_type segments_intersect(side_info const& sides,
                    R const&,
                    coordinate_type const& dx1, coordinate_type const& dy1,
                    coordinate_type const& dx2, coordinate_type const& dy2,
                    S1 const& s1, S2 const& s2)
    {
        bool const ra0 = sides.get<0,0>() == 0;
        bool const ra1 = sides.get<0,1>() == 0;
        bool const rb0 = sides.get<1,0>() == 0;
        bool const rb1 = sides.get<1,1>() == 0;

        return
            // opposite and same starting point (FROM)
            ra0 && rb0 ? calculate_side<1>(sides, dx1, dy1, s1, s2, 'f', -1, -1)

            // opposite and point to each other (TO)
            : ra1 && rb1 ? calculate_side<0>(sides, dx1, dy1, s1, s2, 't', 1, 1)

            // not opposite, forming an angle, first a then b,
            // directed either both left, or both right
            // Check side of B2 from A. This is not calculated before
            : ra1 && rb0 ? angle<1>(sides, dx1, dy1, s1, s2, 'a', 1, -1)

            // not opposite, forming a angle, first b then a,
            // directed either both left, or both right
            : ra0 && rb1 ? angle<0>(sides, dx1, dy1, s1, s2, 'a', -1, 1)

            // b starts from interior of a
            : rb0 ? starts_from_middle(sides, dx1, dy1, s1, s2, 'B', 0, -1)

            // a starts from interior of b (#39)
            : ra0 ? starts_from_middle(sides, dx1, dy1, s1, s2, 'A', -1, 0)

            // b ends at interior of a, calculate direction of A from IP
            : rb1 ? b_ends_at_middle(sides, dx2, dy2, s1, s2)

            // a ends at interior of b
            : ra1 ? a_ends_at_middle(sides, dx1, dy1, s1, s2)

            // normal intersection
            : calculate_side<1>(sides, dx1, dy1, s1, s2, 'i', -1, -1)
            ;
    }

    static inline return_type collinear_touch(
                coordinate_type const& ,
                coordinate_type const& , int arrival_a, int arrival_b)
    {
        // Though this is 'collinear', we handle it as To/From/Angle because it is the same.
        // It only does NOT have a direction.
        side_info sides;
        //int const arrive = how == 'T' ? 1 : -1;
        bool opposite = arrival_a == arrival_b;
        return
            ! opposite
            ? return_type(sides, 'a', arrival_a, arrival_b)
            : return_type(sides, arrival_a == 0 ? 't' : 'f', arrival_a, arrival_b, 0, 0, true);
    }

    template <typename S>
    static inline return_type collinear_interior_boundary_intersect(S const& , bool,
                    int arrival_a, int arrival_b, bool opposite)
    {
        return_type r('c', opposite);
        r.arrival[0] = arrival_a;
        r.arrival[1] = arrival_b;
        return r;
    }

    static inline return_type collinear_a_in_b(S1 const& , bool opposite)
    {
        return_type r('c', opposite);
        r.arrival[0] = 1;
        r.arrival[1] = -1;
        return r;
    }
    static inline return_type collinear_b_in_a(S2 const& , bool opposite)
    {
        return_type r('c', opposite);
        r.arrival[0] = -1;
        r.arrival[1] = 1;
        return r;
    }

    static inline return_type collinear_overlaps(
                    coordinate_type const& , coordinate_type const& ,
                    coordinate_type const& , coordinate_type const& ,
                    int arrival_a, int arrival_b, bool opposite)
    {
        return_type r('c', opposite);
        r.arrival[0] = arrival_a;
        r.arrival[1] = arrival_b;
        return r;
    }

    static inline return_type segment_equal(S1 const& , bool opposite)
    {
        return return_type('e', opposite);
    }

    static inline return_type degenerate(S1 const& , bool)
    {
        return return_type('0', false);
    }

    static inline return_type disjoint()
    {
        return return_type('d', false);
    }

    static inline return_type collinear_disjoint()
    {
        return return_type('d', false);
    }

    static inline return_type error(std::string const&)
    {
        // Return "E" to denote error
        // This will throw an error in get_turn_info
        // TODO: change to enum or similar
        return return_type('E', false);
    }

private :

    static inline bool is_left
        (
            coordinate_type const& ux, 
            coordinate_type const& uy,
            coordinate_type const& vx,
            coordinate_type const& vy
        )
    {
        // This is a "side calculation" as in the strategies, but here terms are precalculated
        // We might merge this with side, offering a pre-calculated term (in fact already done using cross-product)
        // Waiting for implementing spherical...

        rtype const zero = rtype();
        return geometry::detail::determinant<rtype>(ux, uy, vx, vy) > zero;
    }

    template <std::size_t I>
    static inline return_type calculate_side(side_info const& sides,
                coordinate_type const& dx1, coordinate_type const& dy1,
                S1 const& s1, S2 const& s2,
                char how, int how_a, int how_b)
    {
        coordinate_type dpx = get<I, 0>(s2) - get<0, 0>(s1);
        coordinate_type dpy = get<I, 1>(s2) - get<0, 1>(s1);

        return is_left(dx1, dy1, dpx, dpy)
            ? return_type(sides, how, how_a, how_b, -1, 1)
            : return_type(sides, how, how_a, how_b, 1, -1);
    }

    template <std::size_t I>
    static inline return_type angle(side_info const& sides,
                coordinate_type const& dx1, coordinate_type const& dy1,
                S1 const& s1, S2 const& s2,
                char how, int how_a, int how_b)
    {
        coordinate_type dpx = get<I, 0>(s2) - get<0, 0>(s1);
        coordinate_type dpy = get<I, 1>(s2) - get<0, 1>(s1);

        return is_left(dx1, dy1, dpx, dpy)
            ? return_type(sides, how, how_a, how_b, 1, 1)
            : return_type(sides, how, how_a, how_b, -1, -1);
    }


    static inline return_type starts_from_middle(side_info const& sides,
                coordinate_type const& dx1, coordinate_type const& dy1,
                S1 const& s1, S2 const& s2,
                char which,
                int how_a, int how_b)
    {
        // Calculate ARROW of b segment w.r.t. s1
        coordinate_type dpx = get<1, 0>(s2) - get<0, 0>(s1);
        coordinate_type dpy = get<1, 1>(s2) - get<0, 1>(s1);

        int dir = is_left(dx1, dy1, dpx, dpy) ? 1 : -1;

        // From other perspective, then reverse
        bool const is_a = which == 'A';
        if (is_a)
        {
            dir = -dir;
        }

        return return_type(sides, 's',
            how_a,
            how_b,
            is_a ? dir : -dir,
            ! is_a ? dir : -dir);
    }



    // To be harmonized
    static inline return_type a_ends_at_middle(side_info const& sides,
                coordinate_type const& dx, coordinate_type const& dy,
                S1 const& s1, S2 const& s2)
    {
        coordinate_type dpx = get<1, 0>(s2) - get<0, 0>(s1);
        coordinate_type dpy = get<1, 1>(s2) - get<0, 1>(s1);

        // Ending at the middle, one ARRIVES, the other one is NEUTRAL
        // (because it both "arrives"  and "departs"  there
        return is_left(dx, dy, dpx, dpy)
            ? return_type(sides, 'm', 1, 0, 1, 1)
            : return_type(sides, 'm', 1, 0, -1, -1);
    }


    static inline return_type b_ends_at_middle(side_info const& sides,
                coordinate_type const& dx, coordinate_type const& dy,
                S1 const& s1, S2 const& s2)
    {
        coordinate_type dpx = get<1, 0>(s1) - get<0, 0>(s2);
        coordinate_type dpy = get<1, 1>(s1) - get<0, 1>(s2);

        return is_left(dx, dy, dpx, dpy)
            ? return_type(sides, 'm', 0, 1, 1, 1)
            : return_type(sides, 'm', 0, 1, -1, -1);
    }

};

}} // namespace policies::relate

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_DIRECTION_HPP
