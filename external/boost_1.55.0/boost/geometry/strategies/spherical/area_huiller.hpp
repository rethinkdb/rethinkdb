// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_SPHERICAL_AREA_HUILLER_HPP
#define BOOST_GEOMETRY_STRATEGIES_SPHERICAL_AREA_HUILLER_HPP



#include <boost/geometry/strategies/spherical/distance_haversine.hpp>

#include <boost/geometry/core/radian_access.hpp>
#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace area
{



/*!
\brief Area calculation by spherical excess / Huiller's formula
\ingroup strategies
\tparam PointOfSegment point type of segments of rings/polygons
\tparam CalculationType \tparam_calculation
\author Barend Gehrels. Adapted from:
- http://www.soe.ucsc.edu/~pang/160/f98/Gems/GemsIV/sph_poly.c
- http://williams.best.vwh.net/avform.htm
\note The version in Gems didn't account for polygons crossing the 180 meridian.
\note This version works for convex and non-convex polygons, for 180 meridian
crossing polygons and for polygons with holes. However, some cases (especially
180 meridian cases) must still be checked.
\note The version which sums angles, which is often seen, doesn't handle non-convex
polygons correctly.
\note The version which sums longitudes, see
http://trs-new.jpl.nasa.gov/dspace/bitstream/2014/40409/1/07-03.pdf, is simple
and works well in most cases but not in 180 meridian crossing cases. This probably
could be solved.

\note This version is made for spherical equatorial coordinate systems

\qbk{

[heading Example]
[area_with_strategy]
[area_with_strategy_output]


[heading See also]
[link geometry.reference.algorithms.area.area_2_with_strategy area (with strategy)]
}

*/
template
<
    typename PointOfSegment,
    typename CalculationType = void
>
class huiller
{
typedef typename boost::mpl::if_c
    <
        boost::is_void<CalculationType>::type::value,
        typename select_most_precise
            <
                typename coordinate_type<PointOfSegment>::type,
                double
            >::type,
        CalculationType
    >::type calculation_type;

protected :
    struct excess_sum
    {
        calculation_type sum;

        // Distances are calculated on unit sphere here
        strategy::distance::haversine<calculation_type> distance_over_unit_sphere;


        inline excess_sum()
            : sum(0)
            , distance_over_unit_sphere(1)
        {}
        inline calculation_type area(calculation_type radius) const
        {
            return - sum * radius * radius;
        }
    };

public :
    typedef calculation_type return_type;
    typedef PointOfSegment segment_point_type;
    typedef excess_sum state_type;

    inline huiller(calculation_type radius = 1.0)
        : m_radius(radius)
    {}

    inline void apply(PointOfSegment const& p1,
                PointOfSegment const& p2,
                excess_sum& state) const
    {
        if (! geometry::math::equals(get<0>(p1), get<0>(p2)))
        {
            calculation_type const half = 0.5;
            calculation_type const two = 2.0;
            calculation_type const four = 4.0;
            calculation_type const two_pi = two * geometry::math::pi<calculation_type>();
            calculation_type const half_pi = half * geometry::math::pi<calculation_type>();

            // Distance p1 p2
            calculation_type a = state.distance_over_unit_sphere.apply(p1, p2);

            // Sides on unit sphere to south pole
            calculation_type b = half_pi - geometry::get_as_radian<1>(p2);
            calculation_type c = half_pi - geometry::get_as_radian<1>(p1);

            // Semi parameter
            calculation_type s = half * (a + b + c);

            // E: spherical excess, using l'Huiller's formula
            // [tg(e / 4)]2   =   tg[s / 2]  tg[(s-a) / 2]  tg[(s-b) / 2]  tg[(s-c) / 2]
            calculation_type E = four * atan(sqrt(geometry::math::abs(tan(s / two)
                    * tan((s - a) / two)
                    * tan((s - b) / two)
                    * tan((s - c) / two))));

            E = geometry::math::abs(E);

            // In right direction: positive, add area. In left direction: negative, subtract area.
            // Longitude comparisons are not so obvious. If one is negative, other is positive,
            // we have to take the dateline into account.
            // TODO: check this / enhance this, should be more robust. See also the "grow" for ll
            // TODO: use minmax or "smaller"/"compare" strategy for this
            calculation_type lon1 = geometry::get_as_radian<0>(p1) < 0
                ? geometry::get_as_radian<0>(p1) + two_pi
                : geometry::get_as_radian<0>(p1);

            calculation_type lon2 = geometry::get_as_radian<0>(p2) < 0
                ? geometry::get_as_radian<0>(p2) + two_pi
                : geometry::get_as_radian<0>(p2);

            if (lon2 < lon1)
            {
                E = -E;
            }

            state.sum += E;
        }
    }

    inline return_type result(excess_sum const& state) const
    {
        return state.area(m_radius);
    }

private :
    /// Radius of the sphere
    calculation_type m_radius;
};

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{


template <typename Point>
struct default_strategy<spherical_equatorial_tag, Point>
{
    typedef strategy::area::huiller<Point> type;
};

// Note: spherical polar coordinate system requires "get_as_radian_equatorial"
/***template <typename Point>
struct default_strategy<spherical_polar_tag, Point>
{
    typedef strategy::area::huiller<Point> type;
};***/

} // namespace services

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::area




}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_SPHERICAL_AREA_HUILLER_HPP
