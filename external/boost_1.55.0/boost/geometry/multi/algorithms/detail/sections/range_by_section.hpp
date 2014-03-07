// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_SECTIONS_RANGE_BY_SECTION_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_SECTIONS_RANGE_BY_SECTION_HPP


#include <boost/assert.hpp>
#include <boost/range.hpp>

#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>
#include <boost/geometry/multi/core/ring_type.hpp>
#include <boost/geometry/algorithms/detail/sections/range_by_section.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace section
{


template
<
    typename MultiGeometry,
    typename Section,
    typename Policy
>
struct full_section_multi
{
    static inline typename ring_return_type<MultiGeometry const>::type apply(
                MultiGeometry const& multi, Section const& section)
    {
        BOOST_ASSERT
            (
                section.ring_id.multi_index >= 0
                && section.ring_id.multi_index < int(boost::size(multi))
            );

        return Policy::apply(multi[section.ring_id.multi_index], section);
    }
};


}} // namespace detail::section
#endif



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename MultiPolygon, typename Section>
struct range_by_section<multi_polygon_tag, MultiPolygon, Section>
    : detail::section::full_section_multi
        <
            MultiPolygon,
            Section,
            detail::section::full_section_polygon
                <
                    typename boost::range_value<MultiPolygon>::type,
                    Section
                >
       >
{};


} // namespace dispatch
#endif




}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_SECTIONS_RANGE_BY_SECTION_HPP
