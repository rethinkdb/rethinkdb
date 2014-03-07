// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_EQUALS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_EQUALS_HPP


#include <cstddef>
#include <vector>

#include <boost/range.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/reverse_dispatch.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/algorithms/detail/disjoint.hpp>
#include <boost/geometry/algorithms/detail/not.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>

// For trivial checks
#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/length.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_coordinate_type.hpp>
#include <boost/geometry/util/select_most_precise.hpp>

#include <boost/geometry/algorithms/detail/equals/collect_vectors.hpp>

#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace equals
{


template
<
    std::size_t Dimension,
    std::size_t DimensionCount
>
struct box_box
{
    template <typename Box1, typename Box2>
    static inline bool apply(Box1 const& box1, Box2 const& box2)
    {
        if (!geometry::math::equals(get<min_corner, Dimension>(box1), get<min_corner, Dimension>(box2))
            || !geometry::math::equals(get<max_corner, Dimension>(box1), get<max_corner, Dimension>(box2)))
        {
            return false;
        }
        return box_box<Dimension + 1, DimensionCount>::apply(box1, box2);
    }
};

template <std::size_t DimensionCount>
struct box_box<DimensionCount, DimensionCount>
{
    template <typename Box1, typename Box2>
    static inline bool apply(Box1 const& , Box2 const& )
    {
        return true;
    }
};


struct area_check
{
    template <typename Geometry1, typename Geometry2>
    static inline bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2)
    {
        return geometry::math::equals(
                geometry::area(geometry1),
                geometry::area(geometry2));
    }
};


struct length_check
{
    template <typename Geometry1, typename Geometry2>
    static inline bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2)
    {
        return geometry::math::equals(
                geometry::length(geometry1),
                geometry::length(geometry2));
    }
};


template <typename TrivialCheck>
struct equals_by_collection
{
    template <typename Geometry1, typename Geometry2>
    static inline bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2)
    {
        if (! TrivialCheck::apply(geometry1, geometry2))
        {
            return false;
        }

        typedef typename geometry::select_most_precise
            <
                typename select_coordinate_type
                    <
                        Geometry1, Geometry2
                    >::type,
                double
            >::type calculation_type;

        typedef std::vector<collected_vector<calculation_type> > v;
        v c1, c2;

        geometry::collect_vectors(c1, geometry1);
        geometry::collect_vectors(c2, geometry2);

        if (boost::size(c1) != boost::size(c2))
        {
            return false;
        }

        std::sort(c1.begin(), c1.end());
        std::sort(c2.begin(), c2.end());

        // Just check if these vectors are equal.
        return std::equal(c1.begin(), c1.end(), c2.begin());
    }
};


}} // namespace detail::equals
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Geometry1,
    typename Geometry2,
    typename Tag1 = typename tag<Geometry1>::type,
    typename Tag2 = typename tag<Geometry2>::type,
    std::size_t DimensionCount = dimension<Geometry1>::type::value,
    bool Reverse = reverse_dispatch<Geometry1, Geometry2>::type::value
>
struct equals: not_implemented<Tag1, Tag2>
{};


// If reversal is needed, perform it
template
<
    typename Geometry1, typename Geometry2,
    typename Tag1, typename Tag2,
    std::size_t DimensionCount
>
struct equals<Geometry1, Geometry2, Tag1, Tag2, DimensionCount, true>
    : equals<Geometry2, Geometry1, Tag2, Tag1, DimensionCount, false>
{
    static inline bool apply(Geometry1 const& g1, Geometry2 const& g2)
    {
        return equals
            <
                Geometry2, Geometry1,
                Tag2, Tag1,
                DimensionCount,
                false
            >::apply(g2, g1);
    }
};


template <typename P1, typename P2, std::size_t DimensionCount, bool Reverse>
struct equals<P1, P2, point_tag, point_tag, DimensionCount, Reverse>
    : geometry::detail::not_
        <
            P1,
            P2,
            detail::disjoint::point_point<P1, P2, 0, DimensionCount>
        >
{};


template <typename Box1, typename Box2, std::size_t DimensionCount, bool Reverse>
struct equals<Box1, Box2, box_tag, box_tag, DimensionCount, Reverse>
    : detail::equals::box_box<0, DimensionCount>
{};


template <typename Ring1, typename Ring2, bool Reverse>
struct equals<Ring1, Ring2, ring_tag, ring_tag, 2, Reverse>
    : detail::equals::equals_by_collection<detail::equals::area_check>
{};


template <typename Polygon1, typename Polygon2, bool Reverse>
struct equals<Polygon1, Polygon2, polygon_tag, polygon_tag, 2, Reverse>
    : detail::equals::equals_by_collection<detail::equals::area_check>
{};


template <typename LineString1, typename LineString2, bool Reverse>
struct equals<LineString1, LineString2, linestring_tag, linestring_tag, 2, Reverse>
    : detail::equals::equals_by_collection<detail::equals::length_check>
{};


template <typename Polygon, typename Ring, bool Reverse>
struct equals<Polygon, Ring, polygon_tag, ring_tag, 2, Reverse>
    : detail::equals::equals_by_collection<detail::equals::area_check>
{};


template <typename Ring, typename Box, bool Reverse>
struct equals<Ring, Box, ring_tag, box_tag, 2, Reverse>
    : detail::equals::equals_by_collection<detail::equals::area_check>
{};


template <typename Polygon, typename Box, bool Reverse>
struct equals<Polygon, Box, polygon_tag, box_tag, 2, Reverse>
    : detail::equals::equals_by_collection<detail::equals::area_check>
{};


template <typename Geometry1, typename Geometry2>
struct devarianted_equals
{
    static inline bool apply(Geometry1 const& geometry1,
                             Geometry2 const& geometry2)
    {
        concept::check_concepts_and_equal_dimensions
        <
            Geometry1 const,
            Geometry2 const
        >();
        return equals<Geometry1, Geometry2>::apply(geometry1, geometry2);
    }
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T), typename Geometry2>
struct devarianted_equals<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>, Geometry2>
{
    struct visitor: static_visitor<bool>
    {
        Geometry2 const& m_geometry2;

        visitor(Geometry2 const& geometry2)
            : m_geometry2(geometry2)
        {}

        template <typename Geometry1>
        inline bool operator()(Geometry1 const& geometry1) const
        {
            return devarianted_equals<Geometry1, Geometry2>
                       ::apply(geometry1, m_geometry2);
        }

    };

    static inline bool apply(
        boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry1,
        Geometry2 const& geometry2
    )
    {
        return apply_visitor(visitor(geometry2), geometry1);
    }
};

template <typename Geometry1, BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct devarianted_equals<Geometry1, boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    struct visitor: static_visitor<bool>
    {
        Geometry1 const& m_geometry1;

        visitor(Geometry1 const& geometry1)
            : m_geometry1(geometry1)
        {}

        template <typename Geometry2>
        inline bool operator()(Geometry2 const& geometry2) const
        {
            return devarianted_equals<Geometry1, Geometry2>
                       ::apply(m_geometry1, geometry2);
        }

    };

    static inline bool apply(
        Geometry1 const& geometry1,
        boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry2
    )
    {
        return apply_visitor(visitor(geometry1), geometry2);
    }
};

template <
    BOOST_VARIANT_ENUM_PARAMS(typename T1),
    BOOST_VARIANT_ENUM_PARAMS(typename T2)
>
struct devarianted_equals<
    boost::variant<BOOST_VARIANT_ENUM_PARAMS(T1)>,
    boost::variant<BOOST_VARIANT_ENUM_PARAMS(T2)>
>
{
    struct visitor: static_visitor<bool>
    {
        template <typename Geometry1, typename Geometry2>
        inline bool operator()(Geometry1 const& geometry1,
                               Geometry2 const& geometry2) const
        {
            return devarianted_equals<Geometry1, Geometry2>
                ::apply(geometry1, geometry2);
        }

    };

    static inline bool apply(
        boost::variant<BOOST_VARIANT_ENUM_PARAMS(T1)> const& geometry1,
        boost::variant<BOOST_VARIANT_ENUM_PARAMS(T2)> const& geometry2
    )
    {
        return apply_visitor(visitor(), geometry1, geometry2);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
\brief \brief_check{are spatially equal}
\details \details_check12{equals, is spatially equal}. Spatially equal means 
    that the same point set is included. A box can therefore be spatially equal
    to a ring or a polygon, or a linestring can be spatially equal to a 
    multi-linestring or a segment. This only works theoretically, not all
    combinations are implemented yet.
\ingroup equals
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\return \return_check2{are spatially equal}

\qbk{[include reference/algorithms/equals.qbk]}

 */
template <typename Geometry1, typename Geometry2>
inline bool equals(Geometry1 const& geometry1, Geometry2 const& geometry2)
{
    return dispatch::devarianted_equals<Geometry1, Geometry2>
               ::apply(geometry1, geometry2);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_EQUALS_HPP

