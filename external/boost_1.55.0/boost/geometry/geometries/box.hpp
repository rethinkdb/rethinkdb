// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRIES_BOX_HPP
#define BOOST_GEOMETRY_GEOMETRIES_BOX_HPP

#include <cstddef>

#include <boost/concept/assert.hpp>

#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/geometries/concepts/point_concept.hpp>



namespace boost { namespace geometry
{

namespace model
{


/*!
    \brief Class box: defines a box made of two describing points
    \ingroup geometries
    \details Box is always described by a min_corner() and a max_corner() point. If another
        rectangle is used, use linear_ring or polygon.
    \note Boxes are for selections and for calculating the envelope of geometries. Not all algorithms
    are implemented for box. Boxes are also used in Spatial Indexes.
    \tparam Point point type. The box takes a point type as template parameter.
    The point type can be any point type.
    It can be 2D but can also be 3D or more dimensional.
    The box can also take a latlong point type as template parameter.
 */

template<typename Point>
class box
{
    BOOST_CONCEPT_ASSERT( (concept::Point<Point>) );

public:

    inline box() {}

    /*!
        \brief Constructor taking the minimum corner point and the maximum corner point
    */
    inline box(Point const& min_corner, Point const& max_corner)
    {
        geometry::convert(min_corner, m_min_corner);
        geometry::convert(max_corner, m_max_corner);
    }

    inline Point const& min_corner() const { return m_min_corner; }
    inline Point const& max_corner() const { return m_max_corner; }

    inline Point& min_corner() { return m_min_corner; }
    inline Point& max_corner() { return m_max_corner; }

private:

    Point m_min_corner;
    Point m_max_corner;
};


} // namespace model


// Traits specializations for box above
#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template <typename Point>
struct tag<model::box<Point> >
{
    typedef box_tag type;
};

template <typename Point>
struct point_type<model::box<Point> >
{
    typedef Point type;
};

template <typename Point, std::size_t Dimension>
struct indexed_access<model::box<Point>, min_corner, Dimension>
{
    typedef typename geometry::coordinate_type<Point>::type coordinate_type;

    static inline coordinate_type get(model::box<Point> const& b)
    {
        return geometry::get<Dimension>(b.min_corner());
    }

    static inline void set(model::box<Point>& b, coordinate_type const& value)
    {
        geometry::set<Dimension>(b.min_corner(), value);
    }
};

template <typename Point, std::size_t Dimension>
struct indexed_access<model::box<Point>, max_corner, Dimension>
{
    typedef typename geometry::coordinate_type<Point>::type coordinate_type;

    static inline coordinate_type get(model::box<Point> const& b)
    {
        return geometry::get<Dimension>(b.max_corner());
    }

    static inline void set(model::box<Point>& b, coordinate_type const& value)
    {
        geometry::set<Dimension>(b.max_corner(), value);
    }
};

} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_GEOMETRIES_BOX_HPP
