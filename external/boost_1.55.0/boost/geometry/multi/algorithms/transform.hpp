// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_TRANSFORM_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_TRANSFORM_HPP

#include <boost/range.hpp>

#include <boost/geometry/core/mutable_range.hpp>
#include <boost/geometry/algorithms/transform.hpp>

#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace transform
{

/*!
    \brief Is able to transform any multi-geometry, calling the single-version as policy
*/
template <typename Policy>
struct transform_multi
{
    template <typename Multi1, typename Multi2, typename S>
    static inline bool apply(Multi1 const& multi1, Multi2& multi2, S const& strategy)
    {
        traits::resize<Multi2>::apply(multi2, boost::size(multi1));

        typename boost::range_iterator<Multi1 const>::type it1
                = boost::begin(multi1);
        typename boost::range_iterator<Multi2>::type it2
                = boost::begin(multi2);

        for (; it1 != boost::end(multi1); ++it1, ++it2)
        {
            if (! Policy::apply(*it1, *it2, strategy))
            {
                return false;
            }
        }

        return true;
    }
};


}} // namespace detail::transform
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Multi1, typename Multi2>
struct transform
    <
        Multi1, Multi2,
        multi_tag, multi_tag
    >
    : detail::transform::transform_multi
        <
            transform
                <
                    typename boost::range_value<Multi1>::type,
                    typename boost::range_value<Multi2>::type
                >
        >
{};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_TRANSFORM_HPP
