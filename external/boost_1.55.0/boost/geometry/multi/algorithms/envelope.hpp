// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_ENVELOPE_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_ENVELOPE_HPP

#include <vector>

#include <boost/range/metafunctions.hpp>


#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/algorithms/envelope.hpp>

#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/core/point_type.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL

namespace detail { namespace envelope
{


struct envelope_multi_linestring
{
    template<typename MultiLinestring, typename Box>
    static inline void apply(MultiLinestring const& mp, Box& mbr)
    {
        assign_inverse(mbr);
        for (typename boost::range_iterator<MultiLinestring const>::type
                    it = mp.begin();
            it != mp.end();
            ++it)
        {
            envelope_range_additional(*it, mbr);
        }
    }
};


// version for multi_polygon: outer ring's of all polygons
struct envelope_multi_polygon
{
    template<typename MultiPolygon, typename Box>
    static inline void apply(MultiPolygon const& mp, Box& mbr)
    {
        assign_inverse(mbr);
        for (typename boost::range_const_iterator<MultiPolygon>::type
                    it = mp.begin();
            it != mp.end();
            ++it)
        {
            envelope_range_additional(exterior_ring(*it), mbr);
        }
    }
};


}} // namespace detail::envelope

#endif


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Multi>
struct envelope<Multi, multi_point_tag>
    : detail::envelope::envelope_range
{};

template <typename Multi>
struct envelope<Multi, multi_linestring_tag>
    : detail::envelope::envelope_multi_linestring
{};


template <typename Multi>
struct envelope<Multi, multi_polygon_tag>
    : detail::envelope::envelope_multi_polygon
{};


} // namespace dispatch
#endif


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_ENVELOPE_HPP
