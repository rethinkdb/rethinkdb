// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef GEOMETRY_TEST_TEST_GEOMETRIES_WRAPPED_BOOST_ARRAY_HPP
#define GEOMETRY_TEST_TEST_GEOMETRIES_WRAPPED_BOOST_ARRAY_HPP

#include <cstddef>

#include <boost/array.hpp>
#include <boost/range.hpp>

#include <boost/geometry/core/mutable_range.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>


namespace test
{

template <typename Point, std::size_t Count>
struct wrapped_boost_array
{
    inline wrapped_boost_array() : size(0) {}

    boost::array<Point, Count> array;
    int size;
};


} // namespace test


// 1a: adapt to Boost.Range
namespace boost
{
    using namespace test;

    template <typename Point, std::size_t Count>
    struct range_mutable_iterator<wrapped_boost_array<Point, Count> >
        : public range_mutable_iterator<boost::array<Point, Count> >
    {};

    template <typename Point, std::size_t Count>
    struct range_const_iterator<wrapped_boost_array<Point, Count> >
        : public range_const_iterator<boost::array<Point, Count> >
    {};


} // namespace 'boost'


// 1b) adapt to Boost.Range with ADP
namespace test
{
    template <typename Point, std::size_t Count>
    inline typename boost::range_iterator
        <
            wrapped_boost_array<Point, Count>
        >::type range_begin(wrapped_boost_array<Point, Count>& ar)
    {
        return ar.array.begin();
    }

    template <typename Point, std::size_t Count>
    inline typename boost::range_iterator
        <
            wrapped_boost_array<Point, Count> const
        >::type range_begin(wrapped_boost_array<Point, Count> const& ar)
    {
        return ar.array.begin();
    }

    template <typename Point, std::size_t Count>
    inline typename boost::range_iterator
        <
            wrapped_boost_array<Point, Count>
        >::type range_end(wrapped_boost_array<Point, Count>& ar)
    {
        typename boost::range_iterator
            <
                wrapped_boost_array<Point, Count>
            >::type it = ar.array.begin();
        return it + ar.size;
    }

    template <typename Point, std::size_t Count>
    inline typename boost::range_iterator
        <
            wrapped_boost_array<Point, Count> const
        >::type range_end(wrapped_boost_array<Point, Count> const& ar)
    {
        typename boost::range_iterator
            <
                wrapped_boost_array<Point, Count> const
            >::type it = ar.array.begin();
        return it + ar.size;
    }

}


// 2: adapt to Boost.Geometry
namespace boost { namespace geometry { namespace traits
{

    template <typename Point, std::size_t Count>
    struct tag< wrapped_boost_array<Point, Count> >
    {
        typedef linestring_tag type;
    };

    template <typename Point, std::size_t Count>
    struct clear< wrapped_boost_array<Point, Count> >
    {
        static inline void apply(wrapped_boost_array<Point, Count>& ar)
        {
            ar.size = 0;
        }
    };

    template <typename Point, std::size_t Count>
    struct push_back< wrapped_boost_array<Point, Count> >
    {
        static inline void apply(wrapped_boost_array<Point, Count>& ar, Point const& point)
        {
            // BOOST_ASSERT((ar.size < Count));
            ar.array[ar.size++] = point;
        }
    };

    template <typename Point, std::size_t Count>
    struct resize< wrapped_boost_array<Point, Count> >
    {
        static inline void apply(wrapped_boost_array<Point, Count>& ar, std::size_t new_size)
        {
            BOOST_ASSERT(new_size < Count);
            ar.size = new_size;
        }
    };

}}} // namespace bg::traits


#endif // GEOMETRY_TEST_TEST_GEOMETRIES_WRAPPED_BOOST_ARRAY_HPP
