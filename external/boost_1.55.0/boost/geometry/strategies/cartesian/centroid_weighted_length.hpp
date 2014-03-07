// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2009-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_CENTROID_WEIGHTED_LENGTH_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_CENTROID_WEIGHTED_LENGTH_HPP

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/util/select_most_precise.hpp>
#include <boost/geometry/strategies/centroid.hpp>
#include <boost/geometry/strategies/default_distance_result.hpp>

// Helper geometry
#include <boost/geometry/geometries/point.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace centroid
{

namespace detail
{

template <typename Type, std::size_t DimensionCount>
struct weighted_length_sums
{
    typedef typename geometry::model::point
        <
            Type, DimensionCount,
            cs::cartesian
        > work_point;

    Type length;
    work_point average_sum;

    inline weighted_length_sums()
        : length(Type())
    {
        geometry::assign_zero(average_sum);
    }
};
}

template
<
    typename Point,
    typename PointOfSegment = Point
>
class weighted_length
{
private :
    typedef typename select_most_precise
        <
            typename default_distance_result<Point>::type,
            typename default_distance_result<PointOfSegment>::type
        >::type distance_type;

public :
    typedef detail::weighted_length_sums
        <
            distance_type,
            geometry::dimension<Point>::type::value
        > state_type;

    static inline void apply(PointOfSegment const& p1,
            PointOfSegment const& p2, state_type& state)
    {
        distance_type const d = geometry::distance(p1, p2);
        state.length += d;

        typename state_type::work_point weighted_median;
        geometry::assign_zero(weighted_median);
        geometry::add_point(weighted_median, p1);
        geometry::add_point(weighted_median, p2);
        geometry::multiply_value(weighted_median, d/2);
        geometry::add_point(state.average_sum, weighted_median);
    }

    static inline bool result(state_type const& state, Point& centroid)
    {
        distance_type const zero = distance_type();
        if (! geometry::math::equals(state.length, zero))
        {
            assign_zero(centroid);
            add_point(centroid, state.average_sum);
            divide_value(centroid, state.length);
            return true;
        }

        return false;
    }

};

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{


// Register this strategy for linear geometries, in all dimensions

template <std::size_t N, typename Point, typename Geometry>
struct default_strategy
<
    cartesian_tag,
    linear_tag,
    N,
    Point,
    Geometry
>
{
    typedef weighted_length
        <
            Point,
            typename point_type<Geometry>::type
        > type;
};


} // namespace services


#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::centroid


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_CENTROID_WEIGHTED_LENGTH_HPP
