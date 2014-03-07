// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_MULTI_CORE_TAGS_HPP
#define BOOST_GEOMETRY_MULTI_CORE_TAGS_HPP

#include <boost/geometry/core/tags.hpp>

namespace boost { namespace geometry
{

/// OGC Multi point identifying tag
struct multi_point_tag : multi_tag, pointlike_tag  {};

/// OGC Multi linestring identifying tag
struct multi_linestring_tag : multi_tag, linear_tag {};

/// OGC Multi polygon identifying tag
struct multi_polygon_tag : multi_tag, polygonal_tag {};

/// OGC Geometry Collection identifying tag
struct geometry_collection_tag : multi_tag {};




/*!
\brief Meta-function to get for a tag of a multi-geometry
    the tag of the corresponding single-geometry
*/
template <typename Tag>
struct single_tag_of
{};

#ifndef DOXYGEN_NO_DETAIL

template <>
struct single_tag_of<multi_point_tag>
{
    typedef point_tag type;
};

template <>
struct single_tag_of<multi_linestring_tag>
{
    typedef linestring_tag type;
};

template <>
struct single_tag_of<multi_polygon_tag>
{
    typedef polygon_tag type;
};

#endif


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_MULTI_CORE_TAGS_HPP
