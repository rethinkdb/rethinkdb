// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_CALCULATE_SUM_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_CALCULATE_SUM_HPP


#include <boost/range.hpp>
#include <boost/typeof/typeof.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{


class calculate_polygon_sum
{
    template <typename ReturnType, typename Policy, typename Rings, typename Strategy>
    static inline ReturnType sum_interior_rings(Rings const& rings, Strategy const& strategy)
    {
        ReturnType sum = ReturnType();
        for (BOOST_AUTO_TPL(it, boost::begin(rings)); it != boost::end(rings); ++it)
        {
            sum += Policy::apply(*it, strategy);
        }
        return sum;
    }

public :
    template <typename ReturnType, typename Policy, typename Polygon, typename Strategy>
    static inline ReturnType apply(Polygon const& poly, Strategy const& strategy)
    {
        return Policy::apply(exterior_ring(poly), strategy)
            + sum_interior_rings<ReturnType, Policy>(interior_rings(poly), strategy)
            ;
    }
};


} // namespace detail
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_CALCULATE_SUM_HPP
