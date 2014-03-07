// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_CROSS_TRACK_HPP
#define BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_CROSS_TRACK_HPP


#include <boost/config.hpp>
#include <boost/concept_check.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/radian_access.hpp>


#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/strategies/concepts/distance_concept.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>

#include <boost/geometry/util/promote_floating_point.hpp>
#include <boost/geometry/util/math.hpp>

#ifdef BOOST_GEOMETRY_DEBUG_CROSS_TRACK
#  include <boost/geometry/io/dsv/write.hpp>
#endif



namespace boost { namespace geometry
{

namespace strategy { namespace distance
{

/*!
\brief Strategy functor for distance point to segment calculation
\ingroup strategies
\details Class which calculates the distance of a point to a segment, for points on a sphere or globe
\see http://williams.best.vwh.net/avform.htm
\tparam CalculationType \tparam_calculation
\tparam Strategy underlying point-point distance strategy, defaults to haversine

\qbk{
[heading See also]
[link geometry.reference.algorithms.distance.distance_3_with_strategy distance (with strategy)]
}

*/
template
<
    typename CalculationType = void,
    typename Strategy = haversine<double, CalculationType>
>
class cross_track
{
public :
    template <typename Point, typename PointOfSegment>
    struct return_type
        : promote_floating_point
          <
              typename select_calculation_type
                  <
                      Point,
                      PointOfSegment,
                      CalculationType
                  >::type
          >
    {};

    inline cross_track()
    {}

    explicit inline cross_track(typename Strategy::radius_type const& r)
        : m_strategy(r)
    {}

    inline cross_track(Strategy const& s)
        : m_strategy(s)
    {}


    // It might be useful in the future
    // to overload constructor with strategy info.
    // crosstrack(...) {}


    template <typename Point, typename PointOfSegment>
    inline typename return_type<Point, PointOfSegment>::type
    apply(Point const& p, PointOfSegment const& sp1, PointOfSegment const& sp2) const
    {

#if !defined(BOOST_MSVC)
        BOOST_CONCEPT_ASSERT
            (
                (concept::PointDistanceStrategy<Strategy, Point, PointOfSegment>)
            );
#endif

        typedef typename return_type<Point, PointOfSegment>::type return_type;

        // http://williams.best.vwh.net/avform.htm#XTE
        return_type d1 = m_strategy.apply(sp1, p);
        return_type d3 = m_strategy.apply(sp1, sp2);

        if (geometry::math::equals(d3, 0.0))
        {
            // "Degenerate" segment, return either d1 or d2
            return d1;
        }

        return_type d2 = m_strategy.apply(sp2, p);

        return_type crs_AD = course(sp1, p);
        return_type crs_AB = course(sp1, sp2);
        return_type crs_BA = crs_AB - geometry::math::pi<return_type>();
        return_type crs_BD = course(sp2, p);
        return_type d_crs1 = crs_AD - crs_AB;
        return_type d_crs2 = crs_BD - crs_BA;

        // d1, d2, d3 are in principle not needed, only the sign matters
        return_type projection1 = cos( d_crs1 ) * d1 / d3;
        return_type projection2 = cos( d_crs2 ) * d2 / d3;

#ifdef BOOST_GEOMETRY_DEBUG_CROSS_TRACK
        std::cout << "Course " << dsv(sp1) << " to " << dsv(p) << " " << crs_AD * geometry::math::r2d << std::endl;
        std::cout << "Course " << dsv(sp1) << " to " << dsv(sp2) << " " << crs_AB * geometry::math::r2d << std::endl;
        std::cout << "Course " << dsv(sp2) << " to " << dsv(p) << " " << crs_BD * geometry::math::r2d << std::endl;
        std::cout << "Projection AD-AB " << projection1 << " : " << d_crs1 * geometry::math::r2d << std::endl;
        std::cout << "Projection BD-BA " << projection2 << " : " << d_crs2 * geometry::math::r2d << std::endl;
#endif

        if(projection1 > 0.0 && projection2 > 0.0)
        {
            return_type XTD = radius() * geometry::math::abs( asin( sin( d1 / radius() ) * sin( d_crs1 ) ));

 #ifdef BOOST_GEOMETRY_DEBUG_CROSS_TRACK
            std::cout << "Projection ON the segment" << std::endl;
            std::cout << "XTD: " << XTD << " d1: " <<  d1  << " d2: " <<  d2  << std::endl;
#endif

            // Return shortest distance, projected point on segment sp1-sp2
            return return_type(XTD);
        }
        else
        {
#ifdef BOOST_GEOMETRY_DEBUG_CROSS_TRACK
            std::cout << "Projection OUTSIDE the segment" << std::endl;
#endif

            // Return shortest distance, project either on point sp1 or sp2
            return return_type( (std::min)( d1 , d2 ) );
        }
    }

    inline typename Strategy::radius_type radius() const
    { return m_strategy.radius(); }

private :

    Strategy m_strategy;

    /// Calculate course (bearing) between two points. Might be moved to a "course formula" ...
    template <typename Point1, typename Point2>
    inline typename return_type<Point1, Point2>::type
    course(Point1 const& p1, Point2 const& p2) const
    {
        typedef typename return_type<Point1, Point2>::type return_type;

        // http://williams.best.vwh.net/avform.htm#Crs
        return_type dlon = get_as_radian<0>(p2) - get_as_radian<0>(p1);
        return_type cos_p2lat = cos(get_as_radian<1>(p2));

        // "An alternative formula, not requiring the pre-computation of d"
        return atan2(sin(dlon) * cos_p2lat,
            cos(get_as_radian<1>(p1)) * sin(get_as_radian<1>(p2))
            - sin(get_as_radian<1>(p1)) * cos_p2lat * cos(dlon));
    }

};



#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <typename CalculationType, typename Strategy>
struct tag<cross_track<CalculationType, Strategy> >
{
    typedef strategy_tag_distance_point_segment type;
};


template <typename CalculationType, typename Strategy, typename P, typename PS>
struct return_type<cross_track<CalculationType, Strategy>, P, PS>
    : cross_track<CalculationType, Strategy>::template return_type<P, PS>
{};


template <typename CalculationType, typename Strategy>
struct comparable_type<cross_track<CalculationType, Strategy> >
{
    // There is no shortcut, so the strategy itself is its comparable type
    typedef cross_track<CalculationType, Strategy>  type;
};


template
<
    typename CalculationType,
    typename Strategy
>
struct get_comparable<cross_track<CalculationType, Strategy> >
{
    typedef typename comparable_type
        <
            cross_track<CalculationType, Strategy>
        >::type comparable_type;
public :
    static inline comparable_type apply(cross_track<CalculationType, Strategy> const& strategy)
    {
        return comparable_type(strategy.radius());
    }
};


template
<
    typename CalculationType,
    typename Strategy,
    typename P, typename PS
>
struct result_from_distance<cross_track<CalculationType, Strategy>, P, PS>
{
private :
    typedef typename cross_track<CalculationType, Strategy>::template return_type<P, PS> return_type;
public :
    template <typename T>
    static inline return_type apply(cross_track<CalculationType, Strategy> const& , T const& distance)
    {
        return distance;
    }
};


template
<
    typename CalculationType,
    typename Strategy
>
struct strategy_point_point<cross_track<CalculationType, Strategy> >
{
    typedef Strategy type;
};



/*

TODO:  spherical polar coordinate system requires "get_as_radian_equatorial<>"

template <typename Point, typename PointOfSegment, typename Strategy>
struct default_strategy
    <
        segment_tag, Point, PointOfSegment,
        spherical_polar_tag, spherical_polar_tag,
        Strategy
    >
{
    typedef cross_track
        <
            void,
            typename boost::mpl::if_
                <
                    boost::is_void<Strategy>,
                    typename default_strategy
                        <
                            point_tag, Point, PointOfSegment,
                            spherical_polar_tag, spherical_polar_tag
                        >::type,
                    Strategy
                >::type
        > type;
};
*/

template <typename Point, typename PointOfSegment, typename Strategy>
struct default_strategy
    <
        segment_tag, Point, PointOfSegment,
        spherical_equatorial_tag, spherical_equatorial_tag,
        Strategy
    >
{
    typedef cross_track
        <
            void,
            typename boost::mpl::if_
                <
                    boost::is_void<Strategy>,
                    typename default_strategy
                        <
                            point_tag, Point, PointOfSegment,
                            spherical_equatorial_tag, spherical_equatorial_tag
                        >::type,
                    Strategy
                >::type
        > type;
};



} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::distance


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


#endif


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_CROSS_TRACK_HPP
