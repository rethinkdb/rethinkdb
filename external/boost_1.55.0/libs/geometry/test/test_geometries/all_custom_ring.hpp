// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef GEOMETRY_TEST_TEST_GEOMETRIES_ALL_CUSTOM_RING_HPP
#define GEOMETRY_TEST_TEST_GEOMETRIES_ALL_CUSTOM_RING_HPP

#include <cstddef>

#include <boost/range.hpp>


#include <boost/geometry/core/mutable_range.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <test_geometries/all_custom_container.hpp>


template <typename P>
class all_custom_ring : public all_custom_container<P>
{};


// Note that the things below are nearly all identical to implementation
// in *linestring, but it seems not possible to re-use this (without macro's)
// (the only thing DIFFERENT is the tag)


// 1. Adapt to Boost.Geometry
namespace boost { namespace geometry
{

namespace traits
{
    template <typename Point>
    struct tag<all_custom_ring<Point> >
    {
        typedef ring_tag type;
    };


    // Implement traits for mutable actions
    // These are all optional traits (normally / default they are implemented
    // conforming std:: functionality)

    template <typename Point>
    struct clear<all_custom_ring<Point> >
    {
        static inline void apply(all_custom_ring<Point>& acr)
        {
            acr.custom_clear();
        }
    };

    template <typename Point>
    struct push_back<all_custom_ring<Point> >
    {
        static inline void apply(all_custom_ring<Point>& acr, Point const& point)
        {
            acr.custom_push_back(point);
        }
    };

    template <typename Point>
    struct resize<all_custom_ring<Point> >
    {
        static inline void apply(all_custom_ring<Point>& acr, std::size_t new_size)
        {
            acr.custom_resize(new_size);
        }
    };

} // namespace traits

}} // namespace boost::geometry


// 2a. Adapt to Boost.Range, meta-functions
namespace boost
{
    template<typename Point>
    struct range_mutable_iterator<all_custom_ring<Point> >
    {
        typedef typename all_custom_ring<Point>::custom_iterator_type type;
    };

    template<typename Point>
    struct range_const_iterator<all_custom_ring<Point> >
    {
        typedef typename all_custom_ring<Point>::custom_const_iterator_type  type;
    };

} // namespace boost


// 2b. Adapt to Boost.Range, part 2, ADP

template<typename Point>
inline typename all_custom_ring<Point>::custom_iterator_type
    range_begin(all_custom_ring<Point>& acr)
{
    return acr.custom_begin();
}

template<typename Point>
inline typename all_custom_ring<Point>::custom_const_iterator_type
    range_begin(all_custom_ring<Point> const& acr)
{
    return acr.custom_begin();
}

template<typename Point>
inline typename all_custom_ring<Point>::custom_iterator_type
    range_end(all_custom_ring<Point>& acr)
{
    return acr.custom_end();
}

template<typename Point>
inline typename all_custom_ring<Point>::custom_const_iterator_type
    range_end(all_custom_ring<Point> const& acr)
{
    return acr.custom_end();
}

// (Optional)
template<typename Point>
inline std::size_t range_calculate_size(all_custom_ring<Point> const& acr)
{
    return acr.custom_size();
}




#endif // GEOMETRY_TEST_TEST_GEOMETRIES_ALL_CUSTOM_RING_HPP
