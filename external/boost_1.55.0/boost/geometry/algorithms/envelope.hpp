// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_ENVELOPE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_ENVELOPE_HPP

#include <boost/range.hpp>

#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/expand.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace envelope
{


/// Calculate envelope of an 2D or 3D segment
struct envelope_expand_one
{
    template<typename Geometry, typename Box>
    static inline void apply(Geometry const& geometry, Box& mbr)
    {
        assign_inverse(mbr);
        geometry::expand(mbr, geometry);
    }
};


/// Iterate through range (also used in multi*)
template<typename Range, typename Box>
inline void envelope_range_additional(Range const& range, Box& mbr)
{
    typedef typename boost::range_iterator<Range const>::type iterator_type;

    for (iterator_type it = boost::begin(range);
        it != boost::end(range);
        ++it)
    {
        geometry::expand(mbr, *it);
    }
}



/// Generic range dispatching struct
struct envelope_range
{
    /// Calculate envelope of range using a strategy
    template <typename Range, typename Box>
    static inline void apply(Range const& range, Box& mbr)
    {
        assign_inverse(mbr);
        envelope_range_additional(range, mbr);
    }
};

}} // namespace detail::envelope
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Geometry,
    typename Tag = typename tag<Geometry>::type
>
struct envelope: not_implemented<Tag>
{};


template <typename Point>
struct envelope<Point, point_tag>
    : detail::envelope::envelope_expand_one
{};


template <typename Box>
struct envelope<Box, box_tag>
    : detail::envelope::envelope_expand_one
{};


template <typename Segment>
struct envelope<Segment, segment_tag>
    : detail::envelope::envelope_expand_one
{};


template <typename Linestring>
struct envelope<Linestring, linestring_tag>
    : detail::envelope::envelope_range
{};


template <typename Ring>
struct envelope<Ring, ring_tag>
    : detail::envelope::envelope_range
{};


template <typename Polygon>
struct envelope<Polygon, polygon_tag>
    : detail::envelope::envelope_range
{
    template <typename Box>
    static inline void apply(Polygon const& poly, Box& mbr)
    {
        // For polygon, inspecting outer ring is sufficient
        detail::envelope::envelope_range::apply(exterior_ring(poly), mbr);
    }

};


} // namespace dispatch
#endif


/*!
\brief \brief_calc{envelope}
\ingroup envelope
\details \details_calc{envelope,\det_envelope}.
\tparam Geometry \tparam_geometry
\tparam Box \tparam_box
\param geometry \param_geometry
\param mbr \param_box \param_set{envelope}

\qbk{[include reference/algorithms/envelope.qbk]}
\qbk{
[heading Example]
[envelope] [envelope_output]
}
*/
template<typename Geometry, typename Box>
inline void envelope(Geometry const& geometry, Box& mbr)
{
    concept::check<Geometry const>();
    concept::check<Box>();

    dispatch::envelope<Geometry>::apply(geometry, mbr);
}


/*!
\brief \brief_calc{envelope}
\ingroup envelope
\details \details_calc{return_envelope,\det_envelope}. \details_return{envelope}
\tparam Box \tparam_box
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\return \return_calc{envelope}

\qbk{[include reference/algorithms/envelope.qbk]}
\qbk{
[heading Example]
[return_envelope] [return_envelope_output]
}
*/
template<typename Box, typename Geometry>
inline Box return_envelope(Geometry const& geometry)
{
    concept::check<Geometry const>();
    concept::check<Box>();

    Box mbr;
    dispatch::envelope<Geometry>::apply(geometry, mbr);
    return mbr;
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_ENVELOPE_HPP
