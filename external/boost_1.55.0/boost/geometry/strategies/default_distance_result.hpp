// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_DEFAULT_DISTANCE_RESULT_HPP
#define BOOST_GEOMETRY_STRATEGIES_DEFAULT_DISTANCE_RESULT_HPP


#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/strategies/distance.hpp>


namespace boost { namespace geometry
{

/*!
\brief Meta-function defining return type of distance function
\ingroup distance
\note The strategy defines the return-type (so this situation is different
    from length, where distance is sqr/sqrt, but length always squared)
 */
template <typename Geometry1, typename Geometry2 = Geometry1>
struct default_distance_result
{
    typedef typename strategy::distance::services::return_type
        <
            typename strategy::distance::services::default_strategy
                <
                    point_tag,
                    typename point_type<Geometry1>::type,
                    typename point_type<Geometry2>::type
                >::type,
            typename point_type<Geometry1>::type,
            typename point_type<Geometry2>::type
        >::type type;
};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_DEFAULT_DISTANCE_RESULT_HPP
