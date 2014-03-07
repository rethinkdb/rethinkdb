// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_SPHERICAL_SSF_HPP
#define BOOST_GEOMETRY_STRATEGIES_SPHERICAL_SSF_HPP

#include <boost/mpl/if.hpp>
#include <boost/type_traits.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/radian_access.hpp>

#include <boost/geometry/util/select_coordinate_type.hpp>
#include <boost/geometry/util/math.hpp>

#include <boost/geometry/strategies/side.hpp>
//#include <boost/geometry/strategies/concepts/side_concept.hpp>


namespace boost { namespace geometry
{


namespace strategy { namespace side
{


/*!
\brief Check at which side of a Great Circle segment a point lies
         left of segment (> 0), right of segment (< 0), on segment (0)
\ingroup strategies
\tparam CalculationType \tparam_calculation
 */
template <typename CalculationType = void>
class spherical_side_formula
{

public :
    template <typename P1, typename P2, typename P>
    static inline int apply(P1 const& p1, P2 const& p2, P const& p)
    {
        typedef typename boost::mpl::if_c
            <
                boost::is_void<CalculationType>::type::value,

                // Select at least a double...
                typename select_most_precise
                    <
                        typename select_most_precise
                            <
                                typename select_most_precise
                                    <
                                        typename coordinate_type<P1>::type,
                                        typename coordinate_type<P2>::type
                                    >::type,
                                typename coordinate_type<P>::type
                            >::type,
                        double
                    >::type,
                CalculationType
            >::type coordinate_type;

        // Convenient shortcuts
        typedef coordinate_type ct;
        ct const lambda1 = get_as_radian<0>(p1);
        ct const delta1 = get_as_radian<1>(p1);
        ct const lambda2 = get_as_radian<0>(p2);
        ct const delta2 = get_as_radian<1>(p2);
        ct const lambda = get_as_radian<0>(p);
        ct const delta = get_as_radian<1>(p);

        // Create temporary points (vectors) on unit a sphere
        ct const cos_delta1 = cos(delta1);
        ct const c1x = cos_delta1 * cos(lambda1);
        ct const c1y = cos_delta1 * sin(lambda1);
        ct const c1z = sin(delta1);

        ct const cos_delta2 = cos(delta2);
        ct const c2x = cos_delta2 * cos(lambda2);
        ct const c2y = cos_delta2 * sin(lambda2);
        ct const c2z = sin(delta2);

        // (Third point is converted directly)
        ct const cos_delta = cos(delta);
        
        // Apply the "Spherical Side Formula" as presented on my blog
        ct const dist 
            = (c1y * c2z - c1z * c2y) * cos_delta * cos(lambda) 
            + (c1z * c2x - c1x * c2z) * cos_delta * sin(lambda)
            + (c1x * c2y - c1y * c2x) * sin(delta);
        
        ct zero = ct();
        return dist > zero ? 1
            : dist < zero ? -1
            : 0;
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

/*template <typename CalculationType>
struct default_strategy<spherical_polar_tag, CalculationType>
{
    typedef spherical_side_formula<CalculationType> type;
};*/

template <typename CalculationType>
struct default_strategy<spherical_equatorial_tag, CalculationType>
{
    typedef spherical_side_formula<CalculationType> type;
};

template <typename CalculationType>
struct default_strategy<geographic_tag, CalculationType>
{
    typedef spherical_side_formula<CalculationType> type;
};

}
#endif

}} // namespace strategy::side

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_SPHERICAL_SSF_HPP
