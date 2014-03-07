// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_IO_WKT_WRITE_HPP
#define BOOST_GEOMETRY_MULTI_IO_WKT_WRITE_HPP

#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>
#include <boost/geometry/multi/io/wkt/detail/prefix.hpp>
#include <boost/geometry/io/wkt/write.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace wkt
{

template <typename Multi, typename StreamPolicy, typename PrefixPolicy>
struct wkt_multi
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
                Multi const& geometry)
    {
        os << PrefixPolicy::apply();
        // TODO: check EMPTY here
        os << "(";

        for (typename boost::range_iterator<Multi const>::type
                    it = boost::begin(geometry);
            it != boost::end(geometry);
            ++it)
        {
            if (it != boost::begin(geometry))
            {
                os << ",";
            }
            StreamPolicy::apply(os, *it);
        }

        os << ")";
    }
};

}} // namespace wkt::impl
#endif

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Multi>
struct wkt<Multi, multi_point_tag>
    : detail::wkt::wkt_multi
        <
            Multi,
            detail::wkt::wkt_point
                <
                    typename boost::range_value<Multi>::type,
                    detail::wkt::prefix_null
                >,
            detail::wkt::prefix_multipoint
        >
{};

template <typename Multi>
struct wkt<Multi, multi_linestring_tag>
    : detail::wkt::wkt_multi
        <
            Multi,
            detail::wkt::wkt_sequence
                <
                    typename boost::range_value<Multi>::type
                >,
            detail::wkt::prefix_multilinestring
        >
{};

template <typename Multi>
struct wkt<Multi, multi_polygon_tag>
    : detail::wkt::wkt_multi
        <
            Multi,
            detail::wkt::wkt_poly
                <
                    typename boost::range_value<Multi>::type,
                    detail::wkt::prefix_null
                >,
            detail::wkt::prefix_multipolygon
        >
{};

} // namespace dispatch
#endif

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_MULTI_IO_WKT_WRITE_HPP
