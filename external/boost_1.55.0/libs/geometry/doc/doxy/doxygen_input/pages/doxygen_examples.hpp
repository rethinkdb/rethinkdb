// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _DOXYGEN_EXAMPLES_HPP
#define _DOXYGEN_EXAMPLES_HPP


/*!


\example 01_point_example.cpp
In most cases the documentation gives small examples of how to use the algorithms or classes.
The point example is a slightly larger example giving an idea of how to use different
algorithms from the library, related to points. It shows
- the usage of include files
- how to declare points, using different coordinate types
- how to construct points, specifying coordinates, initializing to zero or to infinite
- how to compare points to each other
- how points can be streamed as OGC text
- calculating the distance from point to point


*/


//---------------------------------------------------------------------------------------------------

/*!
\example 02_linestring_example.cpp
The linestring example shows how linestrings can be declared and used and shows some more algorithms.
One of the important concepts of the Generic Geometry Library is that it is totally built upon the standard
library, using the standard containers such as std::vector.

A linestring is, as explained elsewhere in this documentation, not much more than a vector of points.
Most algorithms run on linestrings, but can also run on any iterator pair. And all algorithms
on std::vector can be used on geometry::linestring.

The sample shows this, shows some algorithms:
- geometry::envelope
- geometry::length
- geometry::distance
- geometry::simplify
- geometry::for_each
- geometry::intersection

This documentation illustrates the simplify algorithm and the intersection algorithm with some pictures.

The simplify algorithm simplifies a linestring. Simplification means that the less important points
are removed from the line and that the points that are most important for the shape of a line are
kept. Simplification is done using the well known Douglas Peucker algorithm. The library user can
specify the distance or tolerance, which indicates how much the linestring should be simplified.

The image below shows the original and simplified linestring:
\image html simplify_linestring.png
The blue line is the original linestring; the red line is the simplified line which has one point less.
In geographical applications simplification can reduce a linestring to its basic form containing only
10% of its original points.

The intersection algorithm intersects two geometries which each other, delivering a third geometry.
In the case of the example a linestring is intersected with a box. Intersection with a box is often
called a clip. The image below illustrates the intersection.
\image html clip_linestring.png
The yellow line is intersected with the blue box.
The intersection result, painted in red, contains three linestrings.
*/

//---------------------------------------------------------------------------------------------------

/*!
\example 03_polygon_example.cpp
The polygon example shows some examples of what can be done with polygons in the Generic Geometry Library:
* the outer ring and the inner rings
* how to calculate the area of a polygon
* how to get the centroid, and how to get an often more interesting label point
* how to correct the polygon such that it is clockwise and closed
* within: the well-known point in polygon algorithm
* how to use polygons which use another container, or which use different containers for points and for inner rings
* how polygons can be intersected, or clipped, using a clipping box

The illustrations below show the usage of the within algorithm and the intersection algorithm.

The within algorithm results in true if a point lies completly within a polygon. If it lies exactly
on a border it is not considered as within and if it is inside a hole it is also not within the
polygon. This is illustrated below, where only the point in the middle is within the polygon.

\image html within_polygon.png

The clipping algorithm, called intersection, is illustrated below:

\image html clip_polygon.png

The yellow polygon, containing a hole, is clipped with the blue rectangle, resulting in a
multi_polygon of three polygons, drawn in red. The hole is vanished.

include polygon_example.cpp
*/


//---------------------------------------------------------------------------------------------------
 /*!
\example 06_a_transformation_example.cpp
This sample demonstrates the usage of transformations in the Generic Geometry Library.
Behind the screens this is done using with the uBLAS matrix/vector library.

\example 06_b_transformation_example.cpp

*/

//---------------------------------------------------------------------------------------------------

/*!
\example 07_a_graph_route_example.cpp
The graph route example shows how GGL can be combined with Boost.Graph. The sample does the following things:
- it reads roads (included in the distribution, stored on disk in the form of a text file containing geometries and names)
- it reads cities
- it creates a graph from the roads
- it connects each city to the nearest vertex in the graph
- it calculates the shortest route between each pair of cities
- it outputs the distance over the road, and also of the air
- it creates an SVG image with the roads, the cities, and the first calculated route

Note that this example is useful, but it is only an example. It could be built in many different ways.
For example:
- the roads/cities could be read from a database using SOCI, or from a shapefile using shapelib
- it could support oneway roads and roads on different levels (disconnected bridges)
- it currently uses tuples but that could be anything
- etc

The SVG looks like:
\image html 07_graph_route_example_svg.png

The output screen looks like:
\image html 07_graph_route_example_text.png

\example 07_b_graph_route_example.cpp


*/


//---------------------------------------------------------------------------------------------------
 /*!
\example c01_custom_point_example.cpp
This sample demonstrates that custom points can be made as well. This sample contains many points, derived
from boost::tuple, created from scratch, read only points, legacy points, etc.
*/

//---------------------------------------------------------------------------------------------------
 /*!
\example c02_custom_box_example.cpp
Besides custom points, custom boxes are possible as shown in this example.
*/

//---------------------------------------------------------------------------------------------------
/*
\example c03_custom_linestring_example.cpp
GPS tracks are shown in this example: a custom linestring with GPS points
*/

//---------------------------------------------------------------------------------------------------
 /*!
\example c04_a_custom_triangle_example.cpp
The \b custom triangle \b example goes even further and implements a custom ring, where the area calculation
algorithm is optimized for a triangle
*/

//---------------------------------------------------------------------------------------------------
 /*!
\example c04_b_custom_triangle_example.cpp
This second custom triangle example shows an alternative implementation for a custom shape, showing a
partial specialization for the area calculation.
*/

//---------------------------------------------------------------------------------------------------
 /*!
\example c05_custom_point_pointer_example.cpp
This example shows how GGL can be used to adapt a pointer-to-a-point, used e.g. in a linestring
*/

//---------------------------------------------------------------------------------------------------
 /*!
\example c06_custom_polygon_example.cpp
Showing a custom polygon (asked on the list during Formal Review)
*/



//---------------------------------------------------------------------------------------------------
 /*!
\example x01_qt_example.cpp
This sample demonstrates that by usage of concepts, external geometries can be handled
by GGL, just calling by a one-line registration macro. In this case for the Qt Widget Library.

The example, code shown below, results in this window-output:
\image html x01_qt_example_output.png
*/





//---------------------------------------------------------------------------------------------------
 /*!
\example x03_a_soci_example.cpp
First example showing how to get spatial data from a database using SOCI and put them into GGL
*/


//---------------------------------------------------------------------------------------------------
 /*!
\example x03_b_soci_example.cpp
Second example showing how to get polygons from a database using SOCI and put them into GGL, using WKT.
*/


//---------------------------------------------------------------------------------------------------
 /*
\example x03_c_soci_example.cpp
Example showing how to get polygons from PostGIS using SOCI and use them in GGL through WKB
*/


//---------------------------------------------------------------------------------------------------
 /*
\example x03_d_soci_example.cpp
Example showing how to get polygons from PostGIS using SOCI and use them in GGL through WKB

*/

#endif // _DOXYGEN_EXAMPLES_HPP
