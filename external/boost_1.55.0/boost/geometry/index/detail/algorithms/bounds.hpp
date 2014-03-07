// Boost.Geometry Index
//
// n-dimensional bounds
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_BOUNDS_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_BOUNDS_HPP

namespace boost { namespace geometry { namespace index { namespace detail {

namespace dispatch {

template <typename Geometry,
          typename Bounds,
          typename TagGeometry = typename geometry::tag<Geometry>::type,
          typename TagBounds = typename geometry::tag<Bounds>::type>
struct bounds
{
    static inline void apply(Geometry const& g, Bounds & b)
    {
        geometry::convert(g, b);
    }
};

} // namespace dispatch

template <typename Geometry, typename Bounds>
inline void bounds(Geometry const& g, Bounds & b)
{
    concept::check_concepts_and_equal_dimensions<Geometry const, Bounds>();
    dispatch::bounds<Geometry, Bounds>::apply(g, b);
}

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_BOUNDS_HPP
