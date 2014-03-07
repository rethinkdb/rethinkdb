// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_NUM_GEOMETRIES_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_NUM_GEOMETRIES_HPP


#include <cstddef>

#include <boost/range.hpp>

#include <boost/geometry/algorithms/num_geometries.hpp>
#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename MultiGeometry>
struct num_geometries<MultiGeometry, multi_tag>
{
    static inline std::size_t apply(MultiGeometry const& multi_geometry)
    {
        return boost::size(multi_geometry);
    }
};


} // namespace dispatch
#endif


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_NUM_GEOMETRIES_HPP
