// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_DEFAULT_LENGTH_RESULT_HPP
#define BOOST_GEOMETRY_STRATEGIES_DEFAULT_LENGTH_RESULT_HPP


#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/util/select_most_precise.hpp>


namespace boost { namespace geometry
{

/*!
    \brief Meta-function defining return type of length function
    \ingroup length
    \note Length of a line of integer coordinates can be double.
        So we take at least a double. If Big Number types are used,
        we take that type.

 */
template <typename Geometry>
struct default_length_result
{
    typedef typename select_most_precise
        <
            typename coordinate_type<Geometry>::type,
            long double
        >::type type;
};

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_DEFAULT_LENGTH_RESULT_HPP
