// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_RING_PROPERTIES_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_RING_PROPERTIES_HPP


#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/algorithms/detail/point_on_border.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

template <typename Point>
struct ring_properties
{
    typedef Point point_type;
    typedef typename default_area_result<Point>::type area_type;

    // Filled by "select_rings"
    Point point;
    area_type area;

    // Filled by "update_selection_map"
    int within_code;
    bool reversed;

    // Filled/used by "assign_rings"
    bool discarded;
    ring_identifier parent;
    area_type parent_area;
    std::vector<ring_identifier> children;

    inline ring_properties()
        : area(area_type())
        , within_code(-1)
        , reversed(false)
        , discarded(false)
        , parent_area(-1)
    {}

    template <typename RingOrBox>
    inline ring_properties(RingOrBox const& ring_or_box, bool midpoint)
        : within_code(-1)
        , reversed(false)
        , discarded(false)
        , parent_area(-1)
    {
        this->area = geometry::area(ring_or_box);
        geometry::point_on_border(this->point, ring_or_box, midpoint);
    }

    inline area_type get_area() const
    {
        return reversed ? -area : area;
    }
};

}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_RING_PROPERTIES_HPP
