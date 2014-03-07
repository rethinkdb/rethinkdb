// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_BUFFER_HPP
#define BOOST_GEOMETRY_ALGORITHMS_BUFFER_HPP

#include <cstddef>

#include <boost/numeric/conversion/cast.hpp>


#include <boost/geometry/algorithms/clear.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>
#include <boost/geometry/algorithms/detail/disjoint.hpp>
#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace buffer
{

template <typename BoxIn, typename BoxOut, typename T, std::size_t C, std::size_t D, std::size_t N>
struct box_loop
{
    typedef typename coordinate_type<BoxOut>::type coordinate_type;

    static inline void apply(BoxIn const& box_in, T const& distance, BoxOut& box_out)
    {
        coordinate_type d = distance;
        set<C, D>(box_out, get<C, D>(box_in) + d);
        box_loop<BoxIn, BoxOut, T, C, D + 1, N>::apply(box_in, distance, box_out);
    }
};

template <typename BoxIn, typename BoxOut, typename T, std::size_t C, std::size_t N>
struct box_loop<BoxIn, BoxOut, T, C, N, N>
{
    static inline void apply(BoxIn const&, T const&, BoxOut&) {}
};

// Extends a box with the same amount in all directions
template<typename BoxIn, typename BoxOut, typename T>
inline void buffer_box(BoxIn const& box_in, T const& distance, BoxOut& box_out)
{
    assert_dimension_equal<BoxIn, BoxOut>();

    static const std::size_t N = dimension<BoxIn>::value;

    box_loop<BoxIn, BoxOut, T, min_corner, 0, N>::apply(box_in, -distance, box_out);
    box_loop<BoxIn, BoxOut, T, max_corner, 0, N>::apply(box_in, distance, box_out);
}



}} // namespace detail::buffer
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Input,
    typename Output,
    typename TagIn = typename tag<Input>::type,
    typename TagOut = typename tag<Output>::type
>
struct buffer: not_implemented<TagIn, TagOut>
{};


template <typename BoxIn, typename BoxOut>
struct buffer<BoxIn, BoxOut, box_tag, box_tag>
{
    template <typename Distance>
    static inline void apply(BoxIn const& box_in, Distance const& distance,
                Distance const& , BoxIn& box_out)
    {
        detail::buffer::buffer_box(box_in, distance, box_out);
    }
};

// Many things to do. Point is easy, other geometries require self intersections
// For point, note that it should output as a polygon (like the rest). Buffers
// of a set of geometries are often lateron combined using a "dissolve" operation.
// Two points close to each other get a combined kidney shaped buffer then.

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
\brief \brief_calc{buffer}
\ingroup buffer
\details \details_calc{buffer, \det_buffer}.
\tparam Input \tparam_geometry
\tparam Output \tparam_geometry
\tparam Distance \tparam_numeric
\param geometry_in \param_geometry
\param geometry_out \param_geometry
\param distance The distance to be used for the buffer
\param chord_length (optional) The length of the chord's in the generated arcs around points or bends
\note Currently only implemented for box, the trivial case, but still useful

\qbk{[include reference/algorithms/buffer.qbk]}
 */
template <typename Input, typename Output, typename Distance>
inline void buffer(Input const& geometry_in, Output& geometry_out,
            Distance const& distance, Distance const& chord_length = -1)
{
    concept::check<Input const>();
    concept::check<Output>();

    dispatch::buffer
        <
            Input,
            Output
        >::apply(geometry_in, distance, chord_length, geometry_out);
}

/*!
\brief \brief_calc{buffer}
\ingroup buffer
\details \details_calc{return_buffer, \det_buffer}. \details_return{buffer}.
\tparam Input \tparam_geometry
\tparam Output \tparam_geometry
\tparam Distance \tparam_numeric
\param geometry \param_geometry
\param distance The distance to be used for the buffer
\param chord_length (optional) The length of the chord's in the generated arcs
    around points or bends
\return \return_calc{buffer}
 */
template <typename Output, typename Input, typename Distance>
Output return_buffer(Input const& geometry, Distance const& distance, Distance const& chord_length = -1)
{
    concept::check<Input const>();
    concept::check<Output>();

    Output geometry_out;

    dispatch::buffer
        <
            Input,
            Output
        >::apply(geometry, distance, chord_length, geometry_out);

    return geometry_out;
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_BUFFER_HPP
