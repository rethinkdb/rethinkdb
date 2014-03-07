// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_FOR_EACH_RANGE_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_FOR_EACH_RANGE_HPP


#include <boost/range.hpp>
#include <boost/typeof/typeof.hpp>

#include <boost/geometry/algorithms/detail/for_each_range.hpp>

#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace for_each
{


template <typename Multi, typename Actor, bool IsConst>
struct fe_range_multi
{
    static inline void apply(
                    typename add_const_if_c<IsConst, Multi>::type& multi,
                    Actor& actor)
    {
        for(BOOST_AUTO_TPL(it, boost::begin(multi)); it != boost::end(multi); ++it)
        {
            geometry::detail::for_each_range(*it, actor);
        }
    }
};



}} // namespace detail::for_each
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename MultiPoint, typename Actor, bool IsConst>
struct for_each_range<multi_point_tag, MultiPoint, Actor, IsConst>
    : detail::for_each::fe_range_range<MultiPoint, Actor, IsConst>
{};

template <typename Geometry, typename Actor, bool IsConst>
struct for_each_range<multi_linestring_tag, Geometry, Actor, IsConst>
    :
    detail::for_each::fe_range_multi<Geometry, Actor, IsConst>
{};

template <typename Geometry, typename Actor, bool IsConst>
struct for_each_range<multi_polygon_tag, Geometry, Actor, IsConst>
    :
    detail::for_each::fe_range_multi<Geometry, Actor, IsConst>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH



}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_FOR_EACH_RANGE_HPP
