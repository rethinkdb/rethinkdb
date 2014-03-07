// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_MULTI_CORE_RING_TYPE_HPP
#define BOOST_GEOMETRY_MULTI_CORE_RING_TYPE_HPP


#include <boost/range/metafunctions.hpp>

#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/multi/core/tags.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch
{

template <typename MultiPolygon>
struct ring_return_type<multi_polygon_tag, MultiPolygon>
{
    typedef typename ring_return_type
        <
            polygon_tag,
            typename mpl::if_
                <
                    boost::is_const<MultiPolygon>,
                    typename boost::range_value<MultiPolygon>::type const,
                    typename boost::range_value<MultiPolygon>::type
                >::type
        >::type type;
};


template <typename MultiPolygon>
struct ring_type<multi_polygon_tag, MultiPolygon>
{
    typedef typename boost::remove_reference
        <
            typename ring_return_type<multi_polygon_tag, MultiPolygon>::type
        >::type type;
};


} // namespace core_dispatch
#endif


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_CORE_RING_TYPE_HPP
