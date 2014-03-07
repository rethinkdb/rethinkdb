// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_APPEND_NO_DUPS_OR_SPIKES_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_APPEND_NO_DUPS_OR_SPIKES_HPP

#include <boost/range.hpp>

#include <boost/geometry/algorithms/append.hpp>
#include <boost/geometry/algorithms/detail/point_is_spike_or_equal.hpp>
#include <boost/geometry/algorithms/detail/disjoint.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

template <typename Range, typename Point>
inline void append_no_dups_or_spikes(Range& range, Point const& point)
{
#ifdef BOOST_GEOMETRY_DEBUG_INTERSECTION
    std::cout << "  add: ("
        << geometry::get<0>(point) << ", " << geometry::get<1>(point) << ")"
        << std::endl;
#endif

    traits::push_back<Range>::apply(range, point);

    // If a point is equal, or forming a spike, remove the pen-ultimate point because this one caused the spike.
    // If so, the now-new-pen-ultimate point can again cause a spike (possibly at a corner). So keep doing this.
    // Besides spikes it will also avoid duplicates.
    while(boost::size(range) >= 3 
            && point_is_spike_or_equal(point, *(boost::end(range) - 3), *(boost::end(range) - 2)))
    {
        // Use the Concept/traits, so resize and append again
        traits::resize<Range>::apply(range, boost::size(range) - 2);
        traits::push_back<Range>::apply(range, point);
    }

    // There might still be one duplicate not catched by the condition above
    if (boost::size(range) == 2
        && geometry::detail::equals::equals_point_point(*boost::begin(range), point))
    {
        traits::clear<Range>::apply(range);
        traits::push_back<Range>::apply(range, point);
    }
}


}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_APPEND_NO_DUPS_OR_SPIKES_HPP
