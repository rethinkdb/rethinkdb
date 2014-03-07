// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_EQUALS_COLLECT_VECTORS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_EQUALS_COLLECT_VECTORS_HPP


#include <boost/numeric/conversion/cast.hpp>
#include <boost/range.hpp>
#include <boost/typeof/typeof.hpp>

#include <boost/geometry/multi/core/tags.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/util/math.hpp>



namespace boost { namespace geometry
{

// TODO: if Boost.LA of Emil Dotchevski is accepted, adapt this
template <typename T>
struct collected_vector
{
    typedef T type;

    inline collected_vector()
    {}

    inline collected_vector(T const& px, T const& py,
            T const& pdx, T const& pdy)
        : x(px)
        , y(py)
        , dx(pdx)
        , dy(pdy)
        , dx_0(T())
        , dy_0(T())
    {}

    T x, y;
    T dx, dy;
    T dx_0, dy_0;

    // For sorting
    inline bool operator<(collected_vector<T> const& other) const
    {
        if (math::equals(x, other.x))
        {
            if (math::equals(y, other.y))
            {
                if (math::equals(dx, other.dx))
                {
                    return dy < other.dy;
                }
                return dx < other.dx;
            }
            return y < other.y;
        }
        return x < other.x;
    }

    inline bool same_direction(collected_vector<T> const& other) const
    {
        // For high precision arithmetic, we have to be 
        // more relaxed then using ==
        // Because 2/sqrt( (0,0)<->(2,2) ) == 1/sqrt( (0,0)<->(1,1) ) 
        // is not always true (at least, it is not for ttmath)
        return math::equals_with_epsilon(dx, other.dx)
            && math::equals_with_epsilon(dy, other.dy);
    }

    // For std::equals
    inline bool operator==(collected_vector<T> const& other) const
    {
        return math::equals(x, other.x)
            && math::equals(y, other.y)
            && same_direction(other);
    }
};


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace collect_vectors
{


template <typename Range, typename Collection>
struct range_collect_vectors
{
    typedef typename boost::range_value<Collection>::type item_type;
    typedef typename item_type::type calculation_type;

    static inline void apply(Collection& collection, Range const& range)
    {
        if (boost::size(range) < 2)
        {
            return;
        }

        typedef typename boost::range_iterator<Range const>::type iterator;

        bool first = true;
        iterator it = boost::begin(range);

        for (iterator prev = it++;
            it != boost::end(range);
            prev = it++)
        {
            typename boost::range_value<Collection>::type v;

            v.x = get<0>(*prev);
            v.y = get<1>(*prev);
            v.dx = get<0>(*it) - v.x;
            v.dy = get<1>(*it) - v.y;
            v.dx_0 = v.dx;
            v.dy_0 = v.dy;

            // Normalize the vector -> this results in points+direction
            // and is comparible between geometries
            calculation_type magnitude = sqrt(
                boost::numeric_cast<calculation_type>(v.dx * v.dx + v.dy * v.dy));

            // Avoid non-duplicate points (AND division by zero)
            if (magnitude > 0)
            {
                v.dx /= magnitude;
                v.dy /= magnitude;

                // Avoid non-direction changing points
                if (first || ! v.same_direction(collection.back()))
                {
                    collection.push_back(v);
                }
                first = false;
            }
        }

        // If first one has same direction as last one, remove first one
        if (boost::size(collection) > 1 
            && collection.front().same_direction(collection.back()))
        {
            collection.erase(collection.begin());
        }
    }
};


template <typename Box, typename Collection>
struct box_collect_vectors
{
    // Calculate on coordinate type, but if it is integer,
    // then use double
    typedef typename boost::range_value<Collection>::type item_type;
    typedef typename item_type::type calculation_type;

    static inline void apply(Collection& collection, Box const& box)
    {
        typename point_type<Box>::type lower_left, lower_right,
                upper_left, upper_right;
        geometry::detail::assign_box_corners(box, lower_left, lower_right,
                upper_left, upper_right);

        typedef typename boost::range_value<Collection>::type item;

        collection.push_back(item(get<0>(lower_left), get<1>(lower_left), 0, 1));
        collection.push_back(item(get<0>(upper_left), get<1>(upper_left), 1, 0));
        collection.push_back(item(get<0>(upper_right), get<1>(upper_right), 0, -1));
        collection.push_back(item(get<0>(lower_right), get<1>(lower_right), -1, 0));
    }
};


template <typename Polygon, typename Collection>
struct polygon_collect_vectors
{
    static inline void apply(Collection& collection, Polygon const& polygon)
    {
        typedef typename geometry::ring_type<Polygon>::type ring_type;

        typedef range_collect_vectors<ring_type, Collection> per_range;
        per_range::apply(collection, exterior_ring(polygon));

        typename interior_return_type<Polygon const>::type rings
                    = interior_rings(polygon);
        for (BOOST_AUTO_TPL(it, boost::begin(rings)); it != boost::end(rings); ++it)
        {
            per_range::apply(collection, *it);
        }
    }
};


template <typename MultiGeometry, typename Collection, typename SinglePolicy>
struct multi_collect_vectors
{
    static inline void apply(Collection& collection, MultiGeometry const& multi)
    {
        for (typename boost::range_iterator<MultiGeometry const>::type
                it = boost::begin(multi);
            it != boost::end(multi);
            ++it)
        {
            SinglePolicy::apply(collection, *it);
        }
    }
};


}} // namespace detail::collect_vectors
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Tag,
    typename Collection,
    typename Geometry
>
struct collect_vectors
{
    static inline void apply(Collection&, Geometry const&)
    {}
};


template <typename Collection, typename Box>
struct collect_vectors<box_tag, Collection, Box>
    : detail::collect_vectors::box_collect_vectors<Box, Collection>
{};



template <typename Collection, typename Ring>
struct collect_vectors<ring_tag, Collection, Ring>
    : detail::collect_vectors::range_collect_vectors<Ring, Collection>
{};


template <typename Collection, typename LineString>
struct collect_vectors<linestring_tag, Collection, LineString>
    : detail::collect_vectors::range_collect_vectors<LineString, Collection>
{};


template <typename Collection, typename Polygon>
struct collect_vectors<polygon_tag, Collection, Polygon>
    : detail::collect_vectors::polygon_collect_vectors<Polygon, Collection>
{};


template <typename Collection, typename MultiPolygon>
struct collect_vectors<multi_polygon_tag, Collection, MultiPolygon>
    : detail::collect_vectors::multi_collect_vectors
        <
            MultiPolygon,
            Collection,
            detail::collect_vectors::polygon_collect_vectors
            <
                typename boost::range_value<MultiPolygon>::type,
                Collection
            >
        >
{};



} // namespace dispatch
#endif


/*!
    \ingroup collect_vectors
    \tparam Collection Collection type, should be e.g. std::vector<>
    \tparam Geometry geometry type
    \param collection the collection of vectors
    \param geometry the geometry to make collect_vectors
*/
template <typename Collection, typename Geometry>
inline void collect_vectors(Collection& collection, Geometry const& geometry)
{
    concept::check<Geometry const>();

    dispatch::collect_vectors
        <
            typename tag<Geometry>::type,
            Collection,
            Geometry
        >::apply(collection, geometry);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_EQUALS_COLLECT_VECTORS_HPP
