// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRIES_POINT_HPP
#define BOOST_GEOMETRY_GEOMETRIES_POINT_HPP

#include <cstddef>

#include <boost/mpl/int.hpp>
#include <boost/static_assert.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/util/math.hpp>

namespace boost { namespace geometry
{

// Silence warning C4127: conditional expression is constant
#if defined(_MSC_VER)
#pragma warning(push)  
#pragma warning(disable : 4127)
#endif


namespace model
{

/*!
\brief Basic point class, having coordinates defined in a neutral way
\details Defines a neutral point class, fulfilling the Point Concept.
    Library users can use this point class, or use their own point classes.
    This point class is used in most of the samples and tests of Boost.Geometry
    This point class is used occasionally within the library, where a temporary
    point class is necessary.
\ingroup geometries
\tparam CoordinateType \tparam_numeric
\tparam DimensionCount number of coordinates, usually 2 or 3
\tparam CoordinateSystem coordinate system, for example cs::cartesian

\qbk{[include reference/geometries/point.qbk]}
\qbk{before.synopsis, [heading Model of]}
\qbk{before.synopsis, [link geometry.reference.concepts.concept_point Point Concept]}


*/
template
<
    typename CoordinateType,
    std::size_t DimensionCount,
    typename CoordinateSystem
>
class point
{
public:

    /// @brief Default constructor, no initialization
    inline point()
    {}

    /// @brief Constructor to set one, two or three values
    explicit inline point(CoordinateType const& v0, CoordinateType const& v1 = 0, CoordinateType const& v2 = 0)
    {
        if (DimensionCount >= 1) m_values[0] = v0;
        if (DimensionCount >= 2) m_values[1] = v1;
        if (DimensionCount >= 3) m_values[2] = v2;
    }

    /// @brief Get a coordinate
    /// @tparam K coordinate to get
    /// @return the coordinate
    template <std::size_t K>
    inline CoordinateType const& get() const
    {
        BOOST_STATIC_ASSERT(K < DimensionCount);
        return m_values[K];
    }

    /// @brief Set a coordinate
    /// @tparam K coordinate to set
    /// @param value value to set
    template <std::size_t K>
    inline void set(CoordinateType const& value)
    {
        BOOST_STATIC_ASSERT(K < DimensionCount);
        m_values[K] = value;
    }

private:

    CoordinateType m_values[DimensionCount];
};


} // namespace model

// Adapt the point to the concept
#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{
template
<
    typename CoordinateType,
    std::size_t DimensionCount,
    typename CoordinateSystem
>
struct tag<model::point<CoordinateType, DimensionCount, CoordinateSystem> >
{
    typedef point_tag type;
};

template
<
    typename CoordinateType,
    std::size_t DimensionCount,
    typename CoordinateSystem
>
struct coordinate_type<model::point<CoordinateType, DimensionCount, CoordinateSystem> >
{
    typedef CoordinateType type;
};

template
<
    typename CoordinateType,
    std::size_t DimensionCount,
    typename CoordinateSystem
>
struct coordinate_system<model::point<CoordinateType, DimensionCount, CoordinateSystem> >
{
    typedef CoordinateSystem type;
};

template
<
    typename CoordinateType,
    std::size_t DimensionCount,
    typename CoordinateSystem
>
struct dimension<model::point<CoordinateType, DimensionCount, CoordinateSystem> >
    : boost::mpl::int_<DimensionCount>
{};

template
<
    typename CoordinateType,
    std::size_t DimensionCount,
    typename CoordinateSystem,
    std::size_t Dimension
>
struct access<model::point<CoordinateType, DimensionCount, CoordinateSystem>, Dimension>
{
    static inline CoordinateType get(
        model::point<CoordinateType, DimensionCount, CoordinateSystem> const& p)
    {
        return p.template get<Dimension>();
    }

    static inline void set(
        model::point<CoordinateType, DimensionCount, CoordinateSystem>& p,
        CoordinateType const& value)
    {
        p.template set<Dimension>(value);
    }
};

} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_GEOMETRIES_POINT_HPP
