// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_ALGORITHMS_NUM_GEOMETRIES_HPP
#define BOOST_GEOMETRY_ALGORITHMS_NUM_GEOMETRIES_HPP

#include <cstddef>

#include <boost/geometry/algorithms/not_implemented.hpp>

#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/tag_cast.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Geometry,
    typename Tag = typename tag_cast
                            <
                                typename tag<Geometry>::type,
                                single_tag,
                                multi_tag
                            >::type
>
struct num_geometries: not_implemented<Tag>
{};


template <typename Geometry>
struct num_geometries<Geometry, single_tag>
{
    static inline std::size_t apply(Geometry const&)
    {
        return 1;
    }
};



} // namespace dispatch
#endif


/*!
\brief \brief_calc{number of geometries}
\ingroup num_geometries
\details \details_calc{num_geometries, number of geometries}.
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\return \return_calc{number of geometries}

\qbk{[include reference/algorithms/num_geometries.qbk]}
*/
template <typename Geometry>
inline std::size_t num_geometries(Geometry const& geometry)
{
    concept::check<Geometry const>();

    return dispatch::num_geometries<Geometry>::apply(geometry);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_NUM_GEOMETRIES_HPP
