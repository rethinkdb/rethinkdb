// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_AREA_SURVEYOR_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_AREA_SURVEYOR_HPP


#include <boost/mpl/if.hpp>

#include <boost/geometry/arithmetic/determinant.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/util/select_most_precise.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace area
{

/*!
\brief Area calculation for cartesian points
\ingroup strategies
\details Calculates area using the Surveyor's formula, a well-known
    triangulation algorithm
\tparam PointOfSegment \tparam_segment_point
\tparam CalculationType \tparam_calculation

\qbk{
[heading See also]
[link geometry.reference.algorithms.area.area_2_with_strategy area (with strategy)]
}

*/
template
<
    typename PointOfSegment,
    typename CalculationType = void
>
class surveyor
{
public :
    // If user specified a calculation type, use that type,
    //   whatever it is and whatever the point-type is.
    // Else, use the pointtype, but at least double
    typedef typename
        boost::mpl::if_c
        <
            boost::is_void<CalculationType>::type::value,
            typename select_most_precise
            <
                typename coordinate_type<PointOfSegment>::type,
                double
            >::type,
            CalculationType
        >::type return_type;


private :

    class summation
    {
        friend class surveyor;

        return_type sum;
    public :

        inline summation() : sum(return_type())
        {
            // Strategy supports only 2D areas
            assert_dimension<PointOfSegment, 2>();
        }
        inline return_type area() const
        {
            return_type result = sum;
            return_type const two = 2;
            result /= two;
            return result;
        }
    };

public :
    typedef summation state_type;
    typedef PointOfSegment segment_point_type;

    static inline void apply(PointOfSegment const& p1,
                PointOfSegment const& p2,
                summation& state)
    {
        // SUM += x2 * y1 - x1 * y2;
        state.sum += detail::determinant<return_type>(p2, p1);
    }

    static inline return_type result(summation const& state)
    {
        return state.area();
    }

};

#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{
    template <typename Point>
    struct default_strategy<cartesian_tag, Point>
    {
        typedef strategy::area::surveyor<Point> type;
    };

} // namespace services

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::area



}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_AREA_SURVEYOR_HPP
