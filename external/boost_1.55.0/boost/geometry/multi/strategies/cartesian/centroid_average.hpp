// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_STRATEGIES_CARTESIAN_CENTROID_AVERAGE_HPP
#define BOOST_GEOMETRY_MULTI_STRATEGIES_CARTESIAN_CENTROID_AVERAGE_HPP


#include <boost/numeric/conversion/cast.hpp>
#include <boost/type_traits.hpp>

#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/strategies/centroid.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace centroid
{


/*!
\brief Centroid calculation taking average of points
\ingroup strategies
*/
template
<
    typename PointCentroid,
    typename Point = PointCentroid
>
class average
{
private :

    /*! subclass to keep state */
    class sum
    {
        friend class average;
        int count;
        PointCentroid centroid;

    public :
        inline sum()
            : count(0)
        {
            assign_zero(centroid);
        }
    };

public :
    typedef sum state_type;
    typedef PointCentroid centroid_point_type;
    typedef Point point_type;

    static inline void apply(Point const& p, sum& state)
    {
        add_point(state.centroid, p);
        state.count++;
    }

    static inline void result(sum const& state, PointCentroid& centroid)
    {
        centroid = state.centroid;
        divide_value(centroid, state.count);
    }

};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


namespace services
{

template <typename Point, std::size_t DimensionCount, typename Geometry>
struct default_strategy
<
    cartesian_tag,
    pointlike_tag,
    DimensionCount,
    Point,
    Geometry
>
{
    typedef average
        <
            Point,
            typename point_type<Geometry>::type
        > type;
};

} // namespace services

#endif


}} // namespace strategy::centroid


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_STRATEGIES_CARTESIAN_CENTROID_AVERAGE_HPP
