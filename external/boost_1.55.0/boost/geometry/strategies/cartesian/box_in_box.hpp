// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BOX_IN_BOX_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BOX_IN_BOX_HPP


#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/strategies/covered_by.hpp>
#include <boost/geometry/strategies/within.hpp>


namespace boost { namespace geometry { namespace strategy 
{
    
   
namespace within
{

struct box_within_range
{
    template <typename BoxContainedValue, typename BoxContainingValue>
    static inline bool apply(BoxContainedValue const& bed_min
                , BoxContainedValue const& bed_max
                , BoxContainingValue const& bing_min
                , BoxContainingValue const& bing_max)
    {
        return bed_min > bing_min && bed_max < bing_max;
    }
};


struct box_covered_by_range
{
    template <typename BoxContainedValue, typename BoxContainingValue>
    static inline bool apply(BoxContainedValue const& bed_min
                , BoxContainedValue const& bed_max
                , BoxContainingValue const& bing_min
                , BoxContainingValue const& bing_max)
    {
        return bed_min >= bing_min && bed_max <= bing_max;
    }
};


template
<
    typename SubStrategy,
    typename Box1,
    typename Box2,
    std::size_t Dimension,
    std::size_t DimensionCount
>
struct relate_box_box_loop
{
    static inline bool apply(Box1 const& b_contained, Box2 const& b_containing)
    {
        assert_dimension_equal<Box1, Box2>();

        if (! SubStrategy::apply(
                    get<min_corner, Dimension>(b_contained), 
                    get<max_corner, Dimension>(b_contained), 
                    get<min_corner, Dimension>(b_containing), 
                    get<max_corner, Dimension>(b_containing)
                )
            )
        {
            return false;
        }

        return relate_box_box_loop
            <
                SubStrategy,
                Box1, Box2,
                Dimension + 1, DimensionCount
            >::apply(b_contained, b_containing);
    }
};

template
<
    typename SubStrategy,
    typename Box1,
    typename Box2,
    std::size_t DimensionCount
>
struct relate_box_box_loop<SubStrategy, Box1, Box2, DimensionCount, DimensionCount>
{
    static inline bool apply(Box1 const& , Box2 const& )
    {
        return true;
    }
};

template
<
    typename Box1,
    typename Box2,
    typename SubStrategy = box_within_range
>
struct box_in_box
{
    static inline bool apply(Box1 const& box1, Box2 const& box2)
    {
        return relate_box_box_loop
            <
                SubStrategy, 
                Box1, Box2, 0, dimension<Box1>::type::value
            >::apply(box1, box2);
    }
};


} // namespace within


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


namespace within { namespace services
{

template <typename BoxContained, typename BoxContaining>
struct default_strategy
    <
        box_tag, box_tag, 
        box_tag, areal_tag, 
        cartesian_tag, cartesian_tag, 
        BoxContained, BoxContaining
    >
{
    typedef within::box_in_box<BoxContained, BoxContaining> type;
};


}} // namespace within::services

namespace covered_by { namespace services
{

template <typename BoxContained, typename BoxContaining>
struct default_strategy
    <
        box_tag, box_tag, 
        box_tag, areal_tag, 
        cartesian_tag, cartesian_tag, 
        BoxContained, BoxContaining
    >
{
    typedef within::box_in_box
                <
                    BoxContained, BoxContaining,
                    within::box_covered_by_range
                > type;
};

}} // namespace covered_by::services


#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}}} // namespace boost::geometry::strategy

#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BOX_IN_BOX_HPP
