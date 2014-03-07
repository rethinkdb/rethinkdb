// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_NUM_INTERIOR_RINGS_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_NUM_INTERIOR_RINGS_HPP

#include <cstddef>

#include <boost/range.hpp>

#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>
#include <boost/geometry/multi/core/tags.hpp>

#include <boost/geometry/algorithms/num_interior_rings.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename MultiPolygon>
struct num_interior_rings<MultiPolygon, multi_polygon_tag>
{
    static inline std::size_t apply(MultiPolygon const& multi_polygon)
    {
        std::size_t n = 0;
        for (typename boost::range_iterator<MultiPolygon const>::type
            it = boost::begin(multi_polygon);
            it != boost::end(multi_polygon);
            ++it)
        {
            n += geometry::num_interior_rings(*it);
        }
        return n;
    }

};

} // namespace dispatch
#endif


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_NUM_INTERIOR_RINGS_HPP
