// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_MULTI_CORE_GEOMETRY_ID_HPP
#define BOOST_GEOMETRY_MULTI_CORE_GEOMETRY_ID_HPP


#include <boost/mpl/int.hpp>
#include <boost/type_traits.hpp>


#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/geometry_id.hpp>
#include <boost/geometry/multi/core/tags.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch
{

template <>
struct geometry_id<multi_point_tag>      : boost::mpl::int_<4> {};


template <>
struct geometry_id<multi_linestring_tag> : boost::mpl::int_<5> {};


template <>
struct geometry_id<multi_polygon_tag>    : boost::mpl::int_<6> {};


} // namespace core_dispatch
#endif


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_CORE_GEOMETRY_ID_HPP
