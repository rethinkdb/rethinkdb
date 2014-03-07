// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_CHECK_HPP
#define BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_CHECK_HPP


#include <boost/concept_check.hpp>
#include <boost/concept/requires.hpp>

#include <boost/type_traits/is_const.hpp>

#include <boost/geometry/core/tag.hpp>

#include <boost/geometry/geometries/concepts/box_concept.hpp>
#include <boost/geometry/geometries/concepts/linestring_concept.hpp>
#include <boost/geometry/geometries/concepts/point_concept.hpp>
#include <boost/geometry/geometries/concepts/polygon_concept.hpp>
#include <boost/geometry/geometries/concepts/ring_concept.hpp>
#include <boost/geometry/geometries/concepts/segment_concept.hpp>

#include <boost/geometry/algorithms/not_implemented.hpp>

#include <boost/variant/variant_fwd.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace concept_check
{

template <typename Concept>
class check
{
    BOOST_CONCEPT_ASSERT((Concept ));
};

}} // namespace detail::concept_check
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template 
<
    typename Geometry,
    typename GeometryTag = typename geometry::tag<Geometry>::type,
    bool IsConst = boost::is_const<Geometry>::type::value
>
struct check : not_implemented<GeometryTag>
{};


template <typename Geometry>
struct check<Geometry, point_tag, true>
    : detail::concept_check::check<concept::ConstPoint<Geometry> >
{};


template <typename Geometry>
struct check<Geometry, point_tag, false>
    : detail::concept_check::check<concept::Point<Geometry> >
{};


template <typename Geometry>
struct check<Geometry, linestring_tag, true>
    : detail::concept_check::check<concept::ConstLinestring<Geometry> >
{};


template <typename Geometry>
struct check<Geometry, linestring_tag, false>
    : detail::concept_check::check<concept::Linestring<Geometry> >
{};


template <typename Geometry>
struct check<Geometry, ring_tag, true>
    : detail::concept_check::check<concept::ConstRing<Geometry> >
{};


template <typename Geometry>
struct check<Geometry, ring_tag, false>
    : detail::concept_check::check<concept::Ring<Geometry> >
{};

template <typename Geometry>
struct check<Geometry, polygon_tag, true>
    : detail::concept_check::check<concept::ConstPolygon<Geometry> >
{};


template <typename Geometry>
struct check<Geometry, polygon_tag, false>
    : detail::concept_check::check<concept::Polygon<Geometry> >
{};


template <typename Geometry>
struct check<Geometry, box_tag, true>
    : detail::concept_check::check<concept::ConstBox<Geometry> >
{};


template <typename Geometry>
struct check<Geometry, box_tag, false>
    : detail::concept_check::check<concept::Box<Geometry> >
{};


template <typename Geometry>
struct check<Geometry, segment_tag, true>
    : detail::concept_check::check<concept::ConstSegment<Geometry> >
{};


template <typename Geometry>
struct check<Geometry, segment_tag, false>
    : detail::concept_check::check<concept::Segment<Geometry> >
{};


} // namespace dispatch
#endif




namespace concept
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{


template <typename Geometry>
struct checker : dispatch::check<Geometry>
{};

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct checker<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{};

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct checker<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const>
{};


}
#endif // DOXYGEN_NO_DETAIL


/*!
    \brief Checks, in compile-time, the concept of any geometry
    \ingroup concepts
*/
template <typename Geometry>
inline void check()
{
    detail::checker<Geometry> c;
    boost::ignore_unused_variable_warning(c);
}


/*!
    \brief Checks, in compile-time, the concept of two geometries, and if they
        have equal dimensions
    \ingroup concepts
*/
template <typename Geometry1, typename Geometry2>
inline void check_concepts_and_equal_dimensions()
{
    check<Geometry1>();
    check<Geometry2>();
    assert_dimension_equal<Geometry1, Geometry2>();
}


} // namespace concept


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_GEOMETRIES_CONCEPTS_CHECK_HPP
