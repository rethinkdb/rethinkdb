// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_TURN_INFO_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_TURN_INFO_HPP


#include <boost/array.hpp>

#include <boost/geometry/algorithms/detail/overlay/segment_identifier.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


enum operation_type
{
    operation_none,
    operation_union,
    operation_intersection,
    operation_blocked,
    operation_continue,
    operation_opposite
};


enum method_type
{
    method_none,
    method_disjoint,
    method_crosses,
    method_touch,
    method_touch_interior,
    method_collinear,
    method_equal,
    method_error
};


/*!
    \brief Turn operation: operation
    \details Information necessary for traversal phase (a phase
        of the overlay process). The information is gathered during the
        get_turns (segment intersection) phase.
        The class is to be included in the turn_info class, either direct
        or a derived or similar class with more (e.g. enrichment) information.
 */
struct turn_operation
{
    operation_type operation;
    segment_identifier seg_id;
    segment_identifier other_id;

    inline turn_operation()
        : operation(operation_none)
    {}
};


/*!
    \brief Turn information: intersection point, method, and turn information
    \details Information necessary for traversal phase (a phase
        of the overlay process). The information is gathered during the
        get_turns (segment intersection) phase.
    \tparam Point point type of intersection point
    \tparam Operation gives classes opportunity to add additional info
    \tparam Container gives classes opportunity to define how operations are stored
 */
template
<
    typename Point,
    typename Operation = turn_operation,
    typename Container = boost::array<Operation, 2>
>
struct turn_info
{
    typedef Point point_type;
    typedef Operation turn_operation_type;
    typedef Container container_type;

    Point point;
    method_type method;
    bool discarded;


    Container operations;

    inline turn_info()
        : method(method_none)
        , discarded(false)
    {}

    inline bool both(operation_type type) const
    {
        return has12(type, type);
    }
    
    inline bool has(operation_type type) const
    {
        return this->operations[0].operation == type
            || this->operations[1].operation == type;
    }

    inline bool combination(operation_type type1, operation_type type2) const
    {
        return has12(type1, type2) || has12(type2, type1);
    }


    inline bool is_discarded() const { return discarded; }
    inline bool blocked() const
    {
        return both(operation_blocked);
    }
    inline bool opposite() const
    {
        return both(operation_opposite);
    }
    inline bool any_blocked() const
    {
        return has(operation_blocked);
    }


private :
    inline bool has12(operation_type type1, operation_type type2) const
    {
        return this->operations[0].operation == type1
            && this->operations[1].operation == type2
            ;
    }

};


}} // namespace detail::overlay
#endif //DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_TURN_INFO_HPP
