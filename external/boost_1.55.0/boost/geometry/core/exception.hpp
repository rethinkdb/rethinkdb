// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_CORE_EXCEPTION_HPP
#define BOOST_GEOMETRY_CORE_EXCEPTION_HPP

#include <exception>

namespace boost { namespace geometry
{

/*!
\brief Base exception class for Boost.Geometry algorithms
\ingroup core
\details This class is never thrown. All exceptions thrown in Boost.Geometry
    are derived from exception, so it might be convenient to catch it.
*/
class exception : public std::exception
{};


/*!
\brief Empty Input Exception
\ingroup core
\details The empty_input_exception is thrown if free functions, e.g. distance,
    are called with empty geometries, e.g. a linestring
    without points, a polygon without points, an empty multi-geometry.
\qbk{
[heading See also]
\* [link geometry.reference.algorithms.area the area function]
\* [link geometry.reference.algorithms.distance the distance function]
\* [link geometry.reference.algorithms.length the length function]
}
 */
class empty_input_exception : public geometry::exception
{
public:

    inline empty_input_exception() {}

    virtual char const* what() const throw()
    {
        return "Boost.Geometry Empty-Input exception";
    }
};


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_CORE_EXCEPTION_HPP
