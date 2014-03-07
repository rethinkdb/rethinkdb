// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_OVERLAY_TYPE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_OVERLAY_TYPE_HPP



namespace boost { namespace geometry
{

enum overlay_type
{
    overlay_union,
    overlay_intersection,
    overlay_difference,
    overlay_dissolve
};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_OVERLAY_TYPE_HPP
