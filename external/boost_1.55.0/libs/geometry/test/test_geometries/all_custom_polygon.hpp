// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef GEOMETRY_TEST_TEST_GEOMETRIES_ALL_CUSTOM_POLYGON_HPP
#define GEOMETRY_TEST_TEST_GEOMETRIES_ALL_CUSTOM_POLYGON_HPP

#include <cstddef>

#include <boost/range.hpp>


#include <boost/geometry/core/mutable_range.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <test_geometries/all_custom_container.hpp>
#include <test_geometries/all_custom_ring.hpp>


template <typename P>
class all_custom_polygon
{
public :
    typedef all_custom_ring<P> custom_ring_type;
    typedef all_custom_container<custom_ring_type> custom_int_type;

    custom_ring_type& custom_ext() { return m_ext; }
    custom_int_type& custom_int() { return m_int; }

    custom_ring_type const& custom_ext() const { return m_ext; }
    custom_int_type const& custom_int() const { return m_int; }

private :
    custom_ring_type m_ext;
    custom_int_type m_int;
};



namespace boost { namespace geometry
{

namespace traits
{



template <typename Point>
struct tag<all_custom_polygon<Point> >
{
    typedef polygon_tag type;
};

template <typename Point>
struct ring_const_type<all_custom_polygon<Point> >
{
    typedef typename all_custom_polygon<Point>::custom_ring_type const& type;
};

template <typename Point>
struct ring_mutable_type<all_custom_polygon<Point> >
{
    typedef typename all_custom_polygon<Point>::custom_ring_type& type;
};


template <typename Point>
struct interior_const_type<all_custom_polygon<Point> >
{
    typedef typename all_custom_polygon<Point>::custom_int_type const& type;
};

template <typename Point>
struct interior_mutable_type<all_custom_polygon<Point> >
{
    typedef typename all_custom_polygon<Point>::custom_int_type& type;
};



template <typename Point>
struct exterior_ring<all_custom_polygon<Point> >
{
    typedef all_custom_polygon<Point> polygon_type;
    typedef typename polygon_type::custom_ring_type ring_type;

    static inline ring_type& get(polygon_type& p)
    {
        return p.custom_ext();
    }

    static inline ring_type const& get(polygon_type const& p)
    {
        return p.custom_ext();
    }
};

template <typename Point>
struct interior_rings<all_custom_polygon<Point> >
{
    typedef all_custom_polygon<Point> polygon_type;
    typedef typename polygon_type::custom_int_type int_type;

    static inline int_type& get(polygon_type& p)
    {
        return p.custom_int();
    }

    static inline int_type const& get(polygon_type const& p)
    {
        return p.custom_int();
    }
};


} // namespace traits

}} // namespace boost::geometry





#endif // GEOMETRY_TEST_TEST_GEOMETRIES_ALL_CUSTOM_POLYGON_HPP
