// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_CORE_CLOSURE_HPP
#define BOOST_GEOMETRY_MULTI_CORE_CLOSURE_HPP


#include <boost/mpl/assert.hpp>
#include <boost/range.hpp>
#include <boost/type_traits/remove_const.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/multi/core/tags.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch
{

template <typename Multi>
struct closure<multi_point_tag, Multi> : public core_detail::closure::closed {};

template <typename Multi>
struct closure<multi_linestring_tag, Multi> : public core_detail::closure::closed {};

// Specialization for polygon: the closure is the closure of its rings
template <typename MultiPolygon>
struct closure<multi_polygon_tag, MultiPolygon>
{
    static const closure_selector value = core_dispatch::closure
        <
            polygon_tag,
            typename boost::range_value<MultiPolygon>::type
        >::value ;
};


} // namespace core_dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_CORE_CLOSURE_HPP
