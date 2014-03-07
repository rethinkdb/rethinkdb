// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_GEOMETRIES_LINESTRING_HPP
#define BOOST_GEOMETRY_MULTI_GEOMETRIES_LINESTRING_HPP

#include <memory>
#include <vector>

#include <boost/concept/requires.hpp>

#include <boost/geometry/geometries/concepts/linestring_concept.hpp>

#include <boost/geometry/multi/core/tags.hpp>

namespace boost { namespace geometry
{


namespace model
{

/*!
\brief multi_line, a collection of linestring
\details Multi-linestring can be used to group lines belonging to each other,
        e.g. a highway (with interruptions)
\ingroup geometries

\qbk{before.synopsis,
[heading Model of]
[link geometry.reference.concepts.concept_multi_linestring MultiLineString Concept]
}
*/
template
<
    typename LineString,
    template<typename, typename> class Container = std::vector,
    template<typename> class Allocator = std::allocator
>
class multi_linestring : public Container<LineString, Allocator<LineString> >
{
    BOOST_CONCEPT_ASSERT( (concept::Linestring<LineString>) );
};


} // namespace model


#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template
<
    typename LineString,
    template<typename, typename> class Container,
    template<typename> class Allocator
>
struct tag< model::multi_linestring<LineString, Container, Allocator> >
{
    typedef multi_linestring_tag type;
};

} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_MULTI_GEOMETRIES_LINESTRING_HPP
