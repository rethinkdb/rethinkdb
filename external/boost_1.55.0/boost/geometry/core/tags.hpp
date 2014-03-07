// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_CORE_TAGS_HPP
#define BOOST_GEOMETRY_CORE_TAGS_HPP


namespace boost { namespace geometry
{

// Tags defining strategies linked to coordinate systems

/// Tag used for casting spherical/geographic coordinate systems
struct spherical_tag {};


/// Tag indicating Cartesian coordinate system family (cartesian,epsg)
struct cartesian_tag {};

/// Tag indicating Spherical polar coordinate system family
struct spherical_polar_tag : spherical_tag {};

/// Tag indicating Spherical equatorial coordinate system family
struct spherical_equatorial_tag : spherical_tag {};

/// Tag indicating Geographic coordinate system family (geographic)
struct geographic_tag : spherical_tag {};



// Tags defining tag hierarchy

/// For single-geometries (point, linestring, polygon, box, ring, segment)
struct single_tag {};


/// For multiple-geometries (multi_point, multi_linestring, multi_polygon)
struct multi_tag {};

/// For point-like types (point, multi_point)
struct pointlike_tag {};

/// For linear types (linestring, multi-linestring, segment)
struct linear_tag {};

/// For areal types (polygon, multi_polygon, box, ring)
struct areal_tag {};

// Subset of areal types (polygon, multi_polygon, ring)
struct polygonal_tag : areal_tag {}; 

/// For volume types (also box (?), polyhedron)
struct volumetric_tag {};


// Tags defining geometry types


/// "default" tag
struct geometry_not_recognized_tag {};

/// OGC Point identifying tag
struct point_tag : single_tag, pointlike_tag {};

/// OGC Linestring identifying tag
struct linestring_tag : single_tag, linear_tag {};

/// OGC Polygon identifying tag
struct polygon_tag : single_tag, polygonal_tag {};

/// Convenience (linear) ring identifying tag
struct ring_tag : single_tag, polygonal_tag {};

/// Convenience 2D or 3D box (mbr / aabb) identifying tag
struct box_tag : single_tag, areal_tag {};

/// Convenience segment (2-points) identifying tag
struct segment_tag : single_tag, linear_tag {};



}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_CORE_TAGS_HPP
