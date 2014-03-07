// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_AGNOSTIC_POINT_IN_BOX_BY_SIDE_HPP
#define BOOST_GEOMETRY_STRATEGIES_AGNOSTIC_POINT_IN_BOX_BY_SIDE_HPP

#include <boost/array.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/strategies/covered_by.hpp>
#include <boost/geometry/strategies/within.hpp>


namespace boost { namespace geometry { namespace strategy 
{
    
namespace within
{

struct decide_within
{
    static inline bool apply(int side, bool& result)
    {
        if (side != 1)
        {
            result = false;
            return false;
        }
        return true; // continue
    }
};

struct decide_covered_by
{
    static inline bool apply(int side, bool& result)
    {
        if (side != 1)
        {
            result = side >= 0;
            return false;
        }
        return true; // continue
    }
};


template <typename Point, typename Box, typename Decide = decide_within>
struct point_in_box_by_side
{
    typedef typename strategy::side::services::default_strategy
    <
        typename cs_tag<Box>::type
    >::type side_strategy_type;

    static inline bool apply(Point const& point, Box const& box)
    {
        // Create (counterclockwise) array of points, the fifth one closes it
        // Every point should be on the LEFT side (=1), or ON the border (=0),
        // So >= 1 or >= 0
        boost::array<typename point_type<Box>::type, 5> bp;
        geometry::detail::assign_box_corners_oriented<true>(box, bp);
        bp[4] = bp[0];
        
        bool result = true;
        side_strategy_type strategy;
        boost::ignore_unused_variable_warning(strategy);
        
        for (int i = 1; i < 5; i++)
        {
            int const side = strategy.apply(point, bp[i - 1], bp[i]);
            if (! Decide::apply(side, result))
            {
                return result;
            }
        }
    
        return result;
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
        spherical_tag, spherical_tag, 
        Point, Box
    >
{
    typedef within::point_in_box_by_side
                <
                    Point, Box, within::decide_within
                > type;
};



}} // namespace within::services


namespace covered_by { namespace services
{


template <typename Point, typename Box>
struct default_strategy
    <
        point_tag, box_tag, 
        point_tag, areal_tag, 
        spherical_tag, spherical_tag, 
        Point, Box
    >
{
    typedef within::point_in_box_by_side
                <
                    Point, Box, within::decide_covered_by
                > type;
};


}} // namespace covered_by::services


#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}}} // namespace boost::geometry::strategy


#endif // BOOST_GEOMETRY_STRATEGIES_AGNOSTIC_POINT_IN_BOX_BY_SIDE_HPP
