// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2013 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2013 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2013 Mateusz Loskot, London, UK.
// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_POINT_IS_EQUAL_OR_SPIKE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_POINT_IS_EQUAL_OR_SPIKE_HPP

#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/algorithms/detail/convert_point_to_point.hpp>
#include <boost/geometry/strategies/side.hpp>
#include <boost/geometry/util/math.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename Point1, typename Point2, typename Point3>
static inline bool point_is_spike_or_equal(Point1 const& last_point, Point2 const& segment_a, Point3 const& segment_b)
{
    typedef typename strategy::side::services::default_strategy
    <
        typename cs_tag<Point1>::type
    >::type side_strategy;

    typedef Point1 vector_type;

    int const side = side_strategy::apply(last_point, segment_a, segment_b);
    if (side == 0)
    {
        // Last point is collinear w.r.t previous segment.
        // Check if it is equal
        vector_type diff1;
        conversion::convert_point_to_point(last_point, diff1);
        geometry::subtract_point(diff1, segment_b);
        int const sgn_x1 = math::sign(geometry::get<0>(diff1));
        int const sgn_y1 = math::sign(geometry::get<1>(diff1));
        if (sgn_x1 == 0 && sgn_y1 == 0)
        {
            return true;
        }

        // Check if it moves forward
        vector_type diff2;
        conversion::convert_point_to_point(segment_b, diff2);
        geometry::subtract_point(diff2, segment_a);
        int const sgn_x2 = math::sign(geometry::get<0>(diff2));
        int const sgn_y2 = math::sign(geometry::get<1>(diff2));

        return sgn_x1 != sgn_x2 || sgn_y1 != sgn_y2;
    }
    return false;
}

} // namespace detail
#endif

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_POINT_IS_EQUAL_OR_SPIKE_HPP
