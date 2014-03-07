// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef GEOMETRY_TEST_TEST_GEOMETRIES_ALL_CUSTOM_LINESTRING_HPP
#define GEOMETRY_TEST_TEST_GEOMETRIES_ALL_CUSTOM_LINESTRING_HPP

#include <cstddef>

#include <boost/range.hpp>


#include <boost/geometry/core/mutable_range.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <test_geometries/all_custom_container.hpp>


template <typename P>
class all_custom_linestring : public all_custom_container<P>
{};


// 1. Adapt to Boost.Geometry
namespace boost { namespace geometry
{

namespace traits
{
    template <typename Point>
    struct tag<all_custom_linestring<Point> >
    {
        typedef linestring_tag type;
    };


    // Implement traits for the mutable als
    // These are all optional traits (normally / default they are implemented
    // conforming std:: functionality)

    template <typename Point>
    struct clear<all_custom_linestring<Point> >
    {
        static inline void apply(all_custom_linestring<Point>& als)
        {
            als.custom_clear();
        }
    };

    template <typename Point>
    struct push_back<all_custom_linestring<Point> >
    {
        static inline void apply(all_custom_linestring<Point>& als, Point const& point)
        {
            als.custom_push_back(point);
        }
    };

    template <typename Point>
    struct resize<all_custom_linestring<Point> >
    {
        static inline void apply(all_custom_linestring<Point>& als, std::size_t new_size)
        {
            als.custom_resize(new_size);
        }
    };

} // namespace traits

}} // namespace boost::geometry


// 2a. Adapt to Boost.Range, meta-functions
namespace boost
{
    template<typename Point>
    struct range_mutable_iterator<all_custom_linestring<Point> >
    {
        typedef typename all_custom_linestring<Point>::custom_iterator_type type;
    };

    template<typename Point>
    struct range_const_iterator<all_custom_linestring<Point> >
    {
        typedef typename all_custom_linestring<Point>::custom_const_iterator_type  type;
    };

} // namespace boost


// 2b. Adapt to Boost.Range, part 2, ADP

template<typename Point>
inline typename all_custom_linestring<Point>::custom_iterator_type
    range_begin(all_custom_linestring<Point>& als)
{
    return als.custom_begin();
}

template<typename Point>
inline typename all_custom_linestring<Point>::custom_const_iterator_type
    range_begin(all_custom_linestring<Point> const& als)
{
    return als.custom_begin();
}

template<typename Point>
inline typename all_custom_linestring<Point>::custom_iterator_type
    range_end(all_custom_linestring<Point>& als)
{
    return als.custom_end();
}

template<typename Point>
inline typename all_custom_linestring<Point>::custom_const_iterator_type
    range_end(all_custom_linestring<Point> const& als)
{
    return als.custom_end();
}

// (Optional)
template<typename Point>
inline std::size_t range_calculate_size(all_custom_linestring<Point> const& als)
{
    return als.custom_size();
}


// 3. There used to be a std::back_insert adaption but that is now done using push_back


#endif // GEOMETRY_TEST_TEST_GEOMETRIES_ALL_CUSTOM_LINESTRING_HPP
