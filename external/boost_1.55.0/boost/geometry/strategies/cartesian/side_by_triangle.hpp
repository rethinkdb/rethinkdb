// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_SIDE_BY_TRIANGLE_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_SIDE_BY_TRIANGLE_HPP

#include <boost/mpl/if.hpp>
#include <boost/type_traits.hpp>

#include <boost/geometry/arithmetic/determinant.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/util/select_coordinate_type.hpp>
#include <boost/geometry/strategies/side.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace side
{

/*!
\brief Check at which side of a segment a point lies:
    left of segment (> 0), right of segment (< 0), on segment (0)
\ingroup strategies
\tparam CalculationType \tparam_calculation
 */
template <typename CalculationType = void>
class side_by_triangle
{
public :

    // Template member function, because it is not always trivial
    // or convenient to explicitly mention the typenames in the
    // strategy-struct itself.

    // Types can be all three different. Therefore it is
    // not implemented (anymore) as "segment"

    template <typename P1, typename P2, typename P>
    static inline int apply(P1 const& p1, P2 const& p2, P const& p)
    {
        typedef typename boost::mpl::if_c
            <
                boost::is_void<CalculationType>::type::value,
                typename select_most_precise
                    <
                        typename select_most_precise
                            <
                                typename coordinate_type<P1>::type,
                                typename coordinate_type<P2>::type
                            >::type,
                        typename coordinate_type<P>::type
                    >::type,
                CalculationType
            >::type coordinate_type;

        coordinate_type const x = get<0>(p);
        coordinate_type const y = get<1>(p);

        coordinate_type const sx1 = get<0>(p1);
        coordinate_type const sy1 = get<1>(p1);
        coordinate_type const sx2 = get<0>(p2);
        coordinate_type const sy2 = get<1>(p2);

        // Promote float->double, small int->int
        typedef typename select_most_precise
            <
                coordinate_type,
                double
            >::type promoted_type;

        promoted_type const dx = sx2 - sx1;
        promoted_type const dy = sy2 - sy1;
        promoted_type const dpx = x - sx1;
        promoted_type const dpy = y - sy1;

        promoted_type const s 
            = geometry::detail::determinant<promoted_type>
                (
                    dx, dy, 
                    dpx, dpy
                );

        promoted_type const zero = promoted_type();
        return math::equals(s, zero) ? 0 
            : s > zero ? 1 
            : -1;
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <typename CalculationType>
struct default_strategy<cartesian_tag, CalculationType>
{
    typedef side_by_triangle<CalculationType> type;
};

}
#endif

}} // namespace strategy::side

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_SIDE_BY_TRIANGLE_HPP
