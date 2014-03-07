// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_POINT_IN_BOX_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_POINT_IN_BOX_HPP


#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/strategies/covered_by.hpp>
#include <boost/geometry/strategies/within.hpp>


namespace boost { namespace geometry { namespace strategy 
{
    
namespace within
{


struct within_range
{
    template <typename Value1, typename Value2>
    static inline bool apply(Value1 const& value, Value2 const& min_value, Value2 const& max_value)
    {
        return value > min_value && value < max_value;
    }
};


struct covered_by_range
{
    template <typename Value1, typename Value2>
    static inline bool apply(Value1 const& value, Value2 const& min_value, Value2 const& max_value)
    {
        return value >= min_value && value <= max_value;
    }
};


template
<
    typename SubStrategy,
    typename Point,
    typename Box,
    std::size_t Dimension,
    std::size_t DimensionCount
>
struct relate_point_box_loop
{
    static inline bool apply(Point const& point, Box const& box)
    {
        if (! SubStrategy::apply(get<Dimension>(point), 
                    get<min_corner, Dimension>(box), 
                    get<max_corner, Dimension>(box))
            )
        {
            return false;
        }
        
        return relate_point_box_loop
            <
                SubStrategy,
                Point, Box,
                Dimension + 1, DimensionCount
            >::apply(point, box);
    }
};


template
<
    typename SubStrategy,
    typename Point,
    typename Box,
    std::size_t DimensionCount
>
struct relate_point_box_loop<SubStrategy, Point, Box, DimensionCount, DimensionCount>
{
    static inline bool apply(Point const& , Box const& )
    {
        return true;
    }
};


template
<
    typename Point,
    typename Box,
    typename SubStrategy = within_range
>
struct point_in_box
{
    static inline bool apply(Point const& point, Box const& box) 
    {
        return relate_point_box_loop
            <
                SubStrategy,
                Point, Box, 
                0, dimension<Point>::type::value
            >::apply(point, box);
    }
};


} // namespace within


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


namespace within { namespace services
{

template <typename Point, typename Box>
struct default_strategy
    <
        point_tag, box_tag, 
        point_tag, areal_tag, 
        cartesian_tag, cartesian_tag, 
        Point, Box
    >
{
    typedef within::point_in_box<Point, Box> type; 
};


}} // namespace within::services


namespace covered_by { namespace services
{


template <typename Point, typename Box>
struct default_strategy
    <
        point_tag, box_tag, 
        point_tag, areal_tag, 
        cartesian_tag, cartesian_tag, 
        Point, Box
    >
{
    typedef within::point_in_box
                <
                    Point, Box,
                    within::covered_by_range
                > type;
};


}} // namespace covered_by::services


#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}}} // namespace boost::geometry::strategy


#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_POINT_IN_BOX_HPP
