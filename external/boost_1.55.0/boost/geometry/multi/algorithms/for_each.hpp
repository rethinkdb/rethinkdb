// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_FOR_EACH_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_FOR_EACH_HPP


#include <boost/range.hpp>
#include <boost/typeof/typeof.hpp>

#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/core/point_type.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>

#include <boost/geometry/algorithms/for_each.hpp>



namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace for_each
{

// Implementation of multi, for both point and segment,
// just calling the single version.
template <typename Policy>
struct for_each_multi
{
    template <typename MultiGeometry, typename Functor>
    static inline void apply(MultiGeometry& multi, Functor& f)
    {
        for(BOOST_AUTO_TPL(it, boost::begin(multi)); it != boost::end(multi); ++it)
        {
            Policy::apply(*it, f);
        }
    }
};


}} // namespace detail::for_each
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename MultiGeometry>
struct for_each_point<MultiGeometry, multi_tag>
    : detail::for_each::for_each_multi
        <
            // Specify the dispatch of the single-version as policy
            for_each_point
                <
                    typename add_const_if_c
                        <
                            is_const<MultiGeometry>::value,
                            typename boost::range_value<MultiGeometry>::type
                        >::type
                >
        >
{};


template <typename MultiGeometry>
struct for_each_segment<MultiGeometry, multi_tag>
    : detail::for_each::for_each_multi
        <
            // Specify the dispatch of the single-version as policy
            for_each_segment
                <
                    typename add_const_if_c
                        <
                            is_const<MultiGeometry>::value,
                            typename boost::range_value<MultiGeometry>::type
                        >::type
                >
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_FOR_EACH_HPP
