// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_DISTANCE_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_DISTANCE_HPP


#include <boost/numeric/conversion/bounds.hpp>
#include <boost/range.hpp>

#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/core/geometry_id.hpp>
#include <boost/geometry/multi/core/point_type.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/multi/algorithms/num_points.hpp>
#include <boost/geometry/util/select_coordinate_type.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance
{


template<typename Geometry, typename MultiGeometry, typename Strategy>
struct distance_single_to_multi
    : private dispatch::distance
      <
          Geometry,
          typename range_value<MultiGeometry>::type,
          Strategy
      >
{
    typedef typename strategy::distance::services::return_type
                     <
                         Strategy,
                         typename point_type<Geometry>::type,
                         typename point_type<MultiGeometry>::type
                     >::type return_type;

    static inline return_type apply(Geometry const& geometry,
                MultiGeometry const& multi,
                Strategy const& strategy)
    {
        return_type mindist = return_type();
        bool first = true;

        for(typename range_iterator<MultiGeometry const>::type it = boost::begin(multi);
                it != boost::end(multi);
                ++it, first = false)
        {
            return_type dist = dispatch::distance
                <
                    Geometry,
                    typename range_value<MultiGeometry>::type,
                    Strategy
                >::apply(geometry, *it, strategy);

            if (first || dist < mindist)
            {
                mindist = dist;
            }
        }

        return mindist;
    }
};

template<typename Multi1, typename Multi2, typename Strategy>
struct distance_multi_to_multi
    : private distance_single_to_multi
      <
          typename range_value<Multi1>::type,
          Multi2,
          Strategy
      >
{
    typedef typename strategy::distance::services::return_type
                     <
                         Strategy,
                         typename point_type<Multi1>::type,
                         typename point_type<Multi2>::type
                     >::type return_type;

    static inline return_type apply(Multi1 const& multi1,
                Multi2 const& multi2, Strategy const& strategy)
    {
        return_type mindist = return_type();
        bool first = true;

        for(typename range_iterator<Multi1 const>::type it = boost::begin(multi1);
                it != boost::end(multi1);
                ++it, first = false)
        {
            return_type dist = distance_single_to_multi
                <
                    typename range_value<Multi1>::type,
                    Multi2,
                    Strategy
                >::apply(*it, multi2, strategy);
            if (first || dist < mindist)
            {
                mindist = dist;
            }
        }

        return mindist;
    }
};


}} // namespace detail::distance
#endif


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename G1,
    typename G2,
    typename Strategy,
    typename SingleGeometryTag
>
struct distance
<
    G1, G2, Strategy,
    SingleGeometryTag, multi_tag, strategy_tag_distance_point_point,
    false
>
    : detail::distance::distance_single_to_multi<G1, G2, Strategy>
{};

template <typename G1, typename G2, typename Strategy>
struct distance
<
    G1, G2, Strategy,
    multi_tag, multi_tag, strategy_tag_distance_point_point,
    false
>
    : detail::distance::distance_multi_to_multi<G1, G2, Strategy>
{};

} // namespace dispatch
#endif



}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_DISTANCE_HPP
