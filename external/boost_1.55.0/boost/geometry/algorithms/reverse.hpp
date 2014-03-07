// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_REVERSE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_REVERSE_HPP

#include <algorithm>

#include <boost/range.hpp>
#include <boost/typeof/typeof.hpp>

#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace reverse
{


struct range_reverse
{
    template <typename Range>
    static inline void apply(Range& range)
    {
        std::reverse(boost::begin(range), boost::end(range));
    }
};


struct polygon_reverse: private range_reverse
{
    template <typename Polygon>
    static inline void apply(Polygon& polygon)
    {
        typedef typename geometry::ring_type<Polygon>::type ring_type;

        range_reverse::apply(exterior_ring(polygon));

        typename interior_return_type<Polygon>::type rings
                    = interior_rings(polygon);
        for (BOOST_AUTO_TPL(it, boost::begin(rings)); it != boost::end(rings); ++it)
        {
            range_reverse::apply(*it);
        }
    }
};


}} // namespace detail::reverse
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Geometry, typename Tag = typename tag<Geometry>::type>
struct reverse
{
    static inline void apply(Geometry&)
    {}
};


template <typename Ring>
struct reverse<Ring, ring_tag>
    : detail::reverse::range_reverse
{};


template <typename LineString>
struct reverse<LineString, linestring_tag>
    : detail::reverse::range_reverse
{};


template <typename Polygon>
struct reverse<Polygon, polygon_tag>
    : detail::reverse::polygon_reverse
{};


} // namespace dispatch
#endif


/*!
\brief Reverses the points within a geometry
\details Generic function to reverse a geometry. It resembles the std::reverse
   functionality, but it takes the geometry type into account. Only for a ring
   or for a linestring it is the same as the std::reverse.
\ingroup reverse
\tparam Geometry \tparam_geometry
\param geometry \param_geometry which will be reversed

\qbk{[include reference/algorithms/reverse.qbk]}
*/
template <typename Geometry>
inline void reverse(Geometry& geometry)
{
    concept::check<Geometry>();

    dispatch::reverse<Geometry>::apply(geometry);
}

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_REVERSE_HPP
