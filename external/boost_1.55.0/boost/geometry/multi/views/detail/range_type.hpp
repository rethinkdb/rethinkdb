// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_VIEWS_DETAIL_RANGE_TYPE_HPP
#define BOOST_GEOMETRY_MULTI_VIEWS_DETAIL_RANGE_TYPE_HPP


#include <boost/range.hpp>

#include <boost/geometry/views/detail/range_type.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

// multi-point acts itself as a range
template <typename Geometry>
struct range_type<multi_point_tag, Geometry>
{
    typedef Geometry type;
};


template <typename Geometry>
struct range_type<multi_linestring_tag, Geometry>
{
    typedef typename boost::range_value<Geometry>::type type;
};


template <typename Geometry>
struct range_type<multi_polygon_tag, Geometry>
{
    // Call its single-version
    typedef typename geometry::detail::range_type
        <
            typename boost::range_value<Geometry>::type
        >::type type;
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_MULTI_VIEWS_DETAIL_RANGE_TYPE_HPP
