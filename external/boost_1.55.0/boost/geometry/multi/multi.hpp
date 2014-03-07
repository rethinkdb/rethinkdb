// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_HPP
#define BOOST_GEOMETRY_MULTI_HPP


#include <boost/geometry/multi/core/closure.hpp>
#include <boost/geometry/multi/core/geometry_id.hpp>
#include <boost/geometry/multi/core/interior_rings.hpp>
#include <boost/geometry/multi/core/is_areal.hpp>
#include <boost/geometry/multi/core/point_order.hpp>
#include <boost/geometry/multi/core/point_type.hpp>
#include <boost/geometry/multi/core/ring_type.hpp>
#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/core/topological_dimension.hpp>

#include <boost/geometry/multi/algorithms/append.hpp>
#include <boost/geometry/multi/algorithms/area.hpp>
#include <boost/geometry/multi/algorithms/centroid.hpp>
#include <boost/geometry/multi/algorithms/clear.hpp>
#include <boost/geometry/multi/algorithms/convert.hpp>
#include <boost/geometry/multi/algorithms/correct.hpp>
#include <boost/geometry/multi/algorithms/covered_by.hpp>
#include <boost/geometry/multi/algorithms/disjoint.hpp>
#include <boost/geometry/multi/algorithms/distance.hpp>
#include <boost/geometry/multi/algorithms/envelope.hpp>
#include <boost/geometry/multi/algorithms/equals.hpp>
#include <boost/geometry/multi/algorithms/for_each.hpp>
#include <boost/geometry/multi/algorithms/intersection.hpp>
#include <boost/geometry/multi/algorithms/length.hpp>
#include <boost/geometry/multi/algorithms/num_geometries.hpp>
#include <boost/geometry/multi/algorithms/num_interior_rings.hpp>
#include <boost/geometry/multi/algorithms/num_points.hpp>
#include <boost/geometry/multi/algorithms/perimeter.hpp>
#include <boost/geometry/multi/algorithms/reverse.hpp>
#include <boost/geometry/multi/algorithms/simplify.hpp>
#include <boost/geometry/multi/algorithms/transform.hpp>
#include <boost/geometry/multi/algorithms/unique.hpp>
#include <boost/geometry/multi/algorithms/within.hpp>

#include <boost/geometry/multi/algorithms/detail/for_each_range.hpp>
#include <boost/geometry/multi/algorithms/detail/modify_with_predicate.hpp>
#include <boost/geometry/multi/algorithms/detail/multi_sum.hpp>

#include <boost/geometry/multi/algorithms/detail/sections/range_by_section.hpp>
#include <boost/geometry/multi/algorithms/detail/sections/sectionalize.hpp>

#include <boost/geometry/multi/algorithms/detail/overlay/copy_segment_point.hpp>
#include <boost/geometry/multi/algorithms/detail/overlay/copy_segments.hpp>
#include <boost/geometry/multi/algorithms/detail/overlay/get_ring.hpp>
#include <boost/geometry/multi/algorithms/detail/overlay/get_turns.hpp>
#include <boost/geometry/multi/algorithms/detail/overlay/select_rings.hpp>
#include <boost/geometry/multi/algorithms/detail/overlay/self_turn_points.hpp>

#include <boost/geometry/multi/geometries/concepts/check.hpp>
#include <boost/geometry/multi/geometries/concepts/multi_point_concept.hpp>
#include <boost/geometry/multi/geometries/concepts/multi_linestring_concept.hpp>
#include <boost/geometry/multi/geometries/concepts/multi_polygon_concept.hpp>

#include <boost/geometry/multi/views/detail/range_type.hpp>
#include <boost/geometry/multi/strategies/cartesian/centroid_average.hpp>

#include <boost/geometry/multi/io/dsv/write.hpp>
#include <boost/geometry/multi/io/wkt/wkt.hpp>


#endif // BOOST_GEOMETRY_MULTI_HPP
