// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_FOR_EACH_RANGE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_FOR_EACH_RANGE_HPP


#include <boost/mpl/assert.hpp>
#include <boost/concept/requires.hpp>

#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tag_cast.hpp>

#include <boost/geometry/util/add_const_if_c.hpp>
#include <boost/geometry/views/box_view.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace for_each
{


template <typename Range, typename Actor, bool IsConst>
struct fe_range_range
{
    static inline void apply(
                    typename add_const_if_c<IsConst, Range>::type& range,
                    Actor& actor)
    {
        actor.apply(range);
    }
};


template <typename Polygon, typename Actor, bool IsConst>
struct fe_range_polygon
{
    static inline void apply(
                    typename add_const_if_c<IsConst, Polygon>::type& polygon,
                    Actor& actor)
    {
        actor.apply(exterior_ring(polygon));

        // TODO: If some flag says true, also do the inner rings.
        // for convex hull, it's not necessary
    }
};

template <typename Box, typename Actor, bool IsConst>
struct fe_range_box
{
    static inline void apply(
                    typename add_const_if_c<IsConst, Box>::type& box,
                    Actor& actor)
    {
        actor.apply(box_view<Box>(box));
    }
};


}} // namespace detail::for_each
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Tag,
    typename Geometry,
    typename Actor,
    bool IsConst
>
struct for_each_range
{
    BOOST_MPL_ASSERT_MSG
        (
            false, NOT_OR_NOT_YET_IMPLEMENTED_FOR_THIS_GEOMETRY_TYPE
            , (types<Geometry>)
        );
};


template <typename Linestring, typename Actor, bool IsConst>
struct for_each_range<linestring_tag, Linestring, Actor, IsConst>
    : detail::for_each::fe_range_range<Linestring, Actor, IsConst>
{};


template <typename Ring, typename Actor, bool IsConst>
struct for_each_range<ring_tag, Ring, Actor, IsConst>
    : detail::for_each::fe_range_range<Ring, Actor, IsConst>
{};


template <typename Polygon, typename Actor, bool IsConst>
struct for_each_range<polygon_tag, Polygon, Actor, IsConst>
    : detail::for_each::fe_range_polygon<Polygon, Actor, IsConst>
{};

template <typename Box, typename Actor, bool IsConst>
struct for_each_range<box_tag, Box, Actor, IsConst>
    : detail::for_each::fe_range_box<Box, Actor, IsConst>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

namespace detail
{

template <typename Geometry, typename Actor>
inline void for_each_range(Geometry const& geometry, Actor& actor)
{
    dispatch::for_each_range
        <
            typename tag<Geometry>::type,
            Geometry,
            Actor,
            true
        >::apply(geometry, actor);
}


}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_FOR_EACH_RANGE_HPP
