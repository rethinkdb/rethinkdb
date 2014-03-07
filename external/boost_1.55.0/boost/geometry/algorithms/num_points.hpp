// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_NUM_POINTS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_NUM_POINTS_HPP

#include <cstddef>

#include <boost/range.hpp>
#include <boost/typeof/typeof.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/tag_cast.hpp>
#include <boost/geometry/algorithms/disjoint.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/variant_fwd.hpp>


namespace boost { namespace geometry
{

// Silence warning C4127: conditional expression is constant
#if defined(_MSC_VER)
#pragma warning(push)  
#pragma warning(disable : 4127)
#endif


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace num_points
{


struct range_count
{
    template <typename Range>
    static inline std::size_t apply(Range const& range, bool add_for_open)
    {
        std::size_t n = boost::size(range);
        if (add_for_open && n > 0)
        {
            if (geometry::closure<Range>::value == open)
            {
                if (geometry::disjoint(*boost::begin(range), *(boost::begin(range) + n - 1)))
                {
                    return n + 1;
                }
            }
        }
        return n;
    }
};

template <std::size_t D>
struct other_count
{
    template <typename Geometry>
    static inline std::size_t apply(Geometry const&, bool)
    {
        return D;
    }
};

struct polygon_count: private range_count
{
    template <typename Polygon>
    static inline std::size_t apply(Polygon const& poly, bool add_for_open)
    {
        std::size_t n = range_count::apply(
                    exterior_ring(poly), add_for_open);

        typename interior_return_type<Polygon const>::type rings
                    = interior_rings(poly);
        for (BOOST_AUTO_TPL(it, boost::begin(rings)); it != boost::end(rings); ++it)
        {
            n += range_count::apply(*it, add_for_open);
        }

        return n;
    }
};

}} // namespace detail::num_points
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Geometry,
    typename Tag = typename tag_cast<typename tag<Geometry>::type, multi_tag>::type
>
struct num_points: not_implemented<Tag>
{};

template <typename Geometry>
struct num_points<Geometry, point_tag>
        : detail::num_points::other_count<1>
{};

template <typename Geometry>
struct num_points<Geometry, box_tag>
        : detail::num_points::other_count<4>
{};

template <typename Geometry>
struct num_points<Geometry, segment_tag>
        : detail::num_points::other_count<2>
{};

template <typename Geometry>
struct num_points<Geometry, linestring_tag>
        : detail::num_points::range_count
{};

template <typename Geometry>
struct num_points<Geometry, ring_tag>
        : detail::num_points::range_count
{};

template <typename Geometry>
struct num_points<Geometry, polygon_tag>
        : detail::num_points::polygon_count
{};

template <typename Geometry>
struct devarianted_num_points
{
    static inline std::size_t apply(Geometry const& geometry,
                                    bool add_for_open)
    {
        return num_points<Geometry>::apply(geometry, add_for_open);
    }
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct devarianted_num_points<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    struct visitor: boost::static_visitor<std::size_t>
    {
        bool m_add_for_open;

        visitor(bool add_for_open): m_add_for_open(add_for_open) {}

        template <typename Geometry>
        typename std::size_t operator()(Geometry const& geometry) const
        {
            return dispatch::num_points<Geometry>::apply(geometry, m_add_for_open);
        }
    };

    static inline std::size_t
    apply(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry,
          bool add_for_open)
    {
        return boost::apply_visitor(visitor(add_for_open), geometry);
    }
};


} // namespace dispatch
#endif


/*!
\brief \brief_calc{number of points}
\ingroup num_points
\details \details_calc{num_points, number of points}.
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\param add_for_open add one for open geometries (i.e. polygon types which are not closed)
\return \return_calc{number of points}

\qbk{[include reference/algorithms/num_points.qbk]}
*/
template <typename Geometry>
inline std::size_t num_points(Geometry const& geometry, bool add_for_open = false)
{
    concept::check<Geometry const>();

    return dispatch::devarianted_num_points<Geometry>::apply(geometry, add_for_open);
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_NUM_POINTS_HPP
