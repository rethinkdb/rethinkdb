// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_MULTI_CORE_INTERIOR_RINGS_HPP
#define BOOST_GEOMETRY_MULTI_CORE_INTERIOR_RINGS_HPP


#include <cstddef>

#include <boost/range.hpp>

#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/multi/core/tags.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch
{


template <typename MultiPolygon>
struct interior_type<multi_polygon_tag, MultiPolygon>
{
    typedef typename core_dispatch::interior_type
        <
            polygon_tag,
            typename boost::range_value<MultiPolygon>::type
        >::type type;
};


} // namespace core_dispatch
#endif



}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_CORE_INTERIOR_RINGS_HPP
