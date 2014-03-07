// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_NUM_POINTS_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_NUM_POINTS_HPP


#include <boost/range.hpp>

#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>
#include <boost/geometry/algorithms/num_points.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace num_points
{


struct multi_count
{
    template <typename MultiGeometry>
    static inline size_t apply(MultiGeometry const& geometry, bool add_for_open)
    {
        typedef typename boost::range_value<MultiGeometry>::type geometry_type;
        typedef typename boost::range_iterator
            <
                MultiGeometry const
            >::type iterator_type;

        std::size_t n = 0;
        for (iterator_type it = boost::begin(geometry);
            it != boost::end(geometry);
            ++it)
        {
            n += dispatch::num_points<geometry_type>::apply(*it, add_for_open);
        }
        return n;
    }
};


}} // namespace detail::num_points
#endif


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Geometry>
struct num_points<Geometry, multi_tag>
    : detail::num_points::multi_count {};


} // namespace dispatch
#endif


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_NUM_POINTS_HPP
