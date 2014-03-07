// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_GEOMETRIES_MULTI_POINT_HPP
#define BOOST_GEOMETRY_MULTI_GEOMETRIES_MULTI_POINT_HPP

#include <memory>
#include <vector>

#include <boost/concept/requires.hpp>

#include <boost/geometry/geometries/concepts/point_concept.hpp>

#include <boost/geometry/multi/core/tags.hpp>

namespace boost { namespace geometry
{

namespace model
{


/*!
\brief multi_point, a collection of points
\ingroup geometries
\tparam Point \tparam_point
\tparam Container \tparam_container
\tparam Allocator \tparam_allocator
\details Multipoint can be used to group points belonging to each other,
        e.g. a constellation, or the result set of an intersection
\qbk{before.synopsis,
[heading Model of]
[link geometry.reference.concepts.concept_multi_point MultiPoint Concept]
}
*/
template
<
    typename Point,
    template<typename, typename> class Container = std::vector,
    template<typename> class Allocator = std::allocator
>
class multi_point : public Container<Point, Allocator<Point> >
{
    BOOST_CONCEPT_ASSERT( (concept::Point<Point>) );

    typedef Container<Point, Allocator<Point> > base_type;

public :
    /// \constructor_default{multi_point}
    inline multi_point()
        : base_type()
    {}

    /// \constructor_begin_end{multi_point}
    template <typename Iterator>
    inline multi_point(Iterator begin, Iterator end)
        : base_type(begin, end)
    {}
};

} // namespace model


#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template
<
    typename Point,
    template<typename, typename> class Container,
    template<typename> class Allocator
>
struct tag< model::multi_point<Point, Container, Allocator> >
{
    typedef multi_point_tag type;
};

} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_MULTI_GEOMETRIES_MULTI_POINT_HPP
