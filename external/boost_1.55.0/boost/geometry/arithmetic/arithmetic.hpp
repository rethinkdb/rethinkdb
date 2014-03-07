// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ARITHMETIC_ARITHMETIC_HPP
#define BOOST_GEOMETRY_ARITHMETIC_ARITHMETIC_HPP

#include <functional>

#include <boost/call_traits.hpp>
#include <boost/concept/requires.hpp>

#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/geometries/concepts/point_concept.hpp>
#include <boost/geometry/util/for_each_coordinate.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{


template <typename P>
struct param
{
    typedef typename boost::call_traits
        <
            typename coordinate_type<P>::type
        >::param_type type;
};


template <typename C, template <typename> class Function>
struct value_operation
{
    C m_value;

    inline value_operation(C const &value)
        : m_value(value)
    {}

    template <typename P, int I>
    inline void apply(P& point) const
    {
        set<I>(point, Function<C>()(get<I>(point), m_value));
    }
};

template <typename PointSrc, template <typename> class Function>
struct point_operation
{
    typedef typename coordinate_type<PointSrc>::type coordinate_type;
    PointSrc const& m_source_point;

    inline point_operation(PointSrc const& point)
        : m_source_point(point)
    {}

    template <typename PointDst, int I>
    inline void apply(PointDst& dest_point) const
    {
        set<I>(dest_point,
            Function<coordinate_type>()(get<I>(dest_point), get<I>(m_source_point)));
    }
};


template <typename C>
struct value_assignment
{
    C m_value;

    inline value_assignment(C const &value)
        : m_value(value)
    {}

    template <typename P, int I>
    inline void apply(P& point) const
    {
        set<I>(point, m_value);
    }
};

template <typename PointSrc>
struct point_assignment
{
    PointSrc const& m_source_point;

    inline point_assignment(PointSrc const& point)
        : m_source_point(point)
    {}

    template <typename PointDst, int I>
    inline void apply(PointDst& dest_point) const
    {
        set<I>(dest_point, get<I>(m_source_point));
    }
};


} // namespace detail
#endif // DOXYGEN_NO_DETAIL

/*!
    \brief Adds the same value to each coordinate of a point
    \ingroup arithmetic
    \details
    \param p point
    \param value value to add
 */
template <typename Point>
inline void add_value(Point& p, typename detail::param<Point>::type value)
{
    BOOST_CONCEPT_ASSERT( (concept::Point<Point>) );

    for_each_coordinate(p, detail::value_operation<typename coordinate_type<Point>::type, std::plus>(value));
}

/*!
    \brief Adds a point to another
    \ingroup arithmetic
    \details The coordinates of the second point will be added to those of the first point.
             The second point is not modified.
    \param p1 first point
    \param p2 second point
 */
template <typename Point1, typename Point2>
inline void add_point(Point1& p1, Point2 const& p2)
{
    BOOST_CONCEPT_ASSERT( (concept::Point<Point2>) );
    BOOST_CONCEPT_ASSERT( (concept::ConstPoint<Point2>) );

    for_each_coordinate(p1, detail::point_operation<Point2, std::plus>(p2));
}

/*!
    \brief Subtracts the same value to each coordinate of a point
    \ingroup arithmetic
    \details
    \param p point
    \param value value to subtract
 */
template <typename Point>
inline void subtract_value(Point& p, typename detail::param<Point>::type value)
{
    BOOST_CONCEPT_ASSERT( (concept::Point<Point>) );

    for_each_coordinate(p, detail::value_operation<typename coordinate_type<Point>::type, std::minus>(value));
}

/*!
    \brief Subtracts a point to another
    \ingroup arithmetic
    \details The coordinates of the second point will be subtracted to those of the first point.
             The second point is not modified.
    \param p1 first point
    \param p2 second point
 */
template <typename Point1, typename Point2>
inline void subtract_point(Point1& p1, Point2 const& p2)
{
    BOOST_CONCEPT_ASSERT( (concept::Point<Point2>) );
    BOOST_CONCEPT_ASSERT( (concept::ConstPoint<Point2>) );

    for_each_coordinate(p1, detail::point_operation<Point2, std::minus>(p2));
}

/*!
    \brief Multiplies each coordinate of a point by the same value
    \ingroup arithmetic
    \details
    \param p point
    \param value value to multiply by
 */
template <typename Point>
inline void multiply_value(Point& p, typename detail::param<Point>::type value)
{
    BOOST_CONCEPT_ASSERT( (concept::Point<Point>) );

    for_each_coordinate(p, detail::value_operation<typename coordinate_type<Point>::type, std::multiplies>(value));
}

/*!
    \brief Multiplies a point by another
    \ingroup arithmetic
    \details The coordinates of the first point will be multiplied by those of the second point.
             The second point is not modified.
    \param p1 first point
    \param p2 second point
    \note This is *not* a dot, cross or wedge product. It is a mere field-by-field multiplication.
 */
template <typename Point1, typename Point2>
inline void multiply_point(Point1& p1, Point2 const& p2)
{
    BOOST_CONCEPT_ASSERT( (concept::Point<Point2>) );
    BOOST_CONCEPT_ASSERT( (concept::ConstPoint<Point2>) );

    for_each_coordinate(p1, detail::point_operation<Point2, std::multiplies>(p2));
}

/*!
    \brief Divides each coordinate of the same point by a value
    \ingroup arithmetic
    \details
    \param p point
    \param value value to divide by
 */
template <typename Point>
inline void divide_value(Point& p, typename detail::param<Point>::type value)
{
    BOOST_CONCEPT_ASSERT( (concept::Point<Point>) );

    for_each_coordinate(p, detail::value_operation<typename coordinate_type<Point>::type, std::divides>(value));
}

/*!
    \brief Divides a point by another
    \ingroup arithmetic
    \details The coordinates of the first point will be divided by those of the second point.
             The second point is not modified.
    \param p1 first point
    \param p2 second point
 */
template <typename Point1, typename Point2>
inline void divide_point(Point1& p1, Point2 const& p2)
{
    BOOST_CONCEPT_ASSERT( (concept::Point<Point2>) );
    BOOST_CONCEPT_ASSERT( (concept::ConstPoint<Point2>) );

    for_each_coordinate(p1, detail::point_operation<Point2, std::divides>(p2));
}

/*!
    \brief Assign each coordinate of a point the same value
    \ingroup arithmetic
    \details
    \param p point
    \param value value to assign
 */
template <typename Point>
inline void assign_value(Point& p, typename detail::param<Point>::type value)
{
    BOOST_CONCEPT_ASSERT( (concept::Point<Point>) );

    for_each_coordinate(p, detail::value_assignment<typename coordinate_type<Point>::type>(value));
}

/*!
    \brief Assign a point with another
    \ingroup arithmetic
    \details The coordinates of the first point will be assigned those of the second point.
             The second point is not modified.
    \param p1 first point
    \param p2 second point
 */
template <typename Point1, typename Point2>
inline void assign_point(Point1& p1, const Point2& p2)
{
    BOOST_CONCEPT_ASSERT( (concept::Point<Point2>) );
    BOOST_CONCEPT_ASSERT( (concept::ConstPoint<Point2>) );

    for_each_coordinate(p1, detail::point_assignment<Point2>(p2));
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ARITHMETIC_ARITHMETIC_HPP
