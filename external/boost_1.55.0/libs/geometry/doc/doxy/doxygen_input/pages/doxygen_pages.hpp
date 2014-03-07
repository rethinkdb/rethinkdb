// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _DOXYGEN_PAGES_HPP
#define _DOXYGEN_PAGES_HPP


//---------------------------------------------------------------------------------------------------

/*!
\page compat Relation to Boost and std libraries

\section boost_compat Boost

The GGL makes use of the following Boost Libraries:
- Boost Range
- Boost Type Traits (e.g. remove_const)
- Boost Concept Check Library
- Boost Numeric Conversion (cast, bounds)
- Boost MPL
- Boost Static Assert
- Boost Iterator
- Boost Smart Pointer (shared_ptr, in the extensions spatial index and projections)
- Boost uBLAS
- Adapted geometries:
  - Boost Array
  - Boost Tuple
- IO and parsing
  - Boost Tokenizer
  - Boost Conversion (lexical cast)
  - Boost String Algo
- Testing
  - Boost Test
  - Boost Timer
- Examples
  - Boost Graph Library


Many of these are used a lot inside the library.

The \b Boost.Range library is used everywhere
to declare iterators and to walk through standard containers or custom ranges.

The \b Boost \b Concept \b Check \b Library is used to check concepts, for geometries as well
as for strategies.

Tranformations are implemented using \b Boost uBLAS

\b MPL is used for a.o. reverse dispatching

Finally, the library can perfectly be used together with the \b Boost \b Graph \b Library, as shown in example 7.


\section std_compat std:: library


The GGL makes substantial use of the std library, and is designed to be compatible with it.
- provided geometries (linestring, ring, polygon, multi-s) make default use of std::vector (but that is configurable)
- std sorting, unique copies, maps can make use of provided functors in compare "compare".
- internally, there are vector's, deque's, there is sorting, many things are implemented using the std library

*/



//---------------------------------------------------------------------------------------------------

/*!
\page compiling Compiling

\section compiling_intro Compiling Boost.Geometry

Boost.Geometry is a Header Only library. So just including the headerfiles is enough
to use the algorithms. Nothing has to be linked.

Boost.Geometry is only dependant on (header only) Boost libraries. Download the Boost
Library Collection from www.boost.org, adapt the include path to include Boost.

\section platforms Platforms

The library currently compiles successfully on the following platforms:
- MS Visual Studio 2008 (Express Edition)
- MS Visual Studio 2005 (Express Edition)
- gcc version 3.4
- gcc version 4 (tested are 4.1, 4.2, 4.4)

For Visual Studio, the examples contains some project files (for 2005). However, because the library
    is header only, it will be no problem to integrate it in your own project files.

The test and example folders contain also Jam-files for bjam / boost-build

For gcc, the flag -Wno-long-long can be used (surpressing warnings originating from Boost)

\section convenient_includes Convenient includes

This section concentrates on how to include GGL.

The most convenient headerfile including all algorithms and strategies is geometry.hpp:
- \#include <boost/geometry/geometry.hpp> It is recommended to include this file.
Alternatively, it is possible to include GGL headerfiles separately. However, this is inconvenient as files might
be renamed or moved occasionaly.


An often used headerfile is geometries.hpp:
- \#include <boost/geometry/geometries/geometries.hpp> This includes all default geometries: point, linestring,
  polygon, linear_ring, box. It is not included in the "geometry.hpp" headerfile because
  users are allowed to use their own geometries. However, for library users who want to use the provided
  geometries it is most useful.


For users using only Cartesian points, with floating point coordinates (double), in 2D or 3D, you can use instead:
- \#include <boost/geometry/geometries/cartesian2d.hpp> This includes all 2D Cartesian geometries: point_2d, linestring_2d,
  etc. Using this headerfile the library seems to be a non-template library, so it is convenient for users that
  are not so into the template world.

For users using multi-geometries:
- \#include <boost/geometry/multi/multi.hpp>

\section advanced_includes Advanced includes

This section is for users who have their own geometries and want to use algorithms from the Boost.Geometry.


If you want to use your own points it makes sense to use the registration macro's:
- \#include <boost/geometry/geometries/register/point.hpp> macro's for point registration
- \#include <boost/geometry/geometries/register/box.hpp> macro's for box registration




\section extension_includes Extensions
The GGL is currently (since 0.5) split into the kernel and several extensions. As this split is relatively new,
there are not yet umbrella-include files, but most things below will do:

If you want to use the \b geographic coordinate system:
- the tag is already included in the kernel
- \#include <boost/geometry/extensions/gis/geographic/strategies/andoyer.hpp> for Andoyer distance calculations
- \#include <boost/geometry/extensions/gis/geographic/strategies/vincenty.hpp> for Vincenty distance calculations

If you want to use the \b projections:
- \#include <boost/geometry/extensions/gis/projections/parameters.hpp>
- \#include <boost/geometry/extensions/gis/projections/projection.hpp>
- \#include <boost/geometry/extensions/gis/projections/factory.hpp>

If you want to use the \b circle (n-sphere)
- \#include <boost/geometry/extensions/nsphere/...> <i>Several headerfiles, there is not yet a common include</i>



\section performance Performance finetuning
The enumeration below is not exhaustive but can contain hints to improve the performance
- For Microsoft, set the define _SECURE_SCL=0
- For Microsoft, set the define _HAS_ITERATOR_DEBUGGING=0
- our measurements indicate that MSVC 2005 generates faster code than MSVC 2008
- Using StlPort results in significant faster code than Microsoft's standard library
- Of course turn on compiler optimizations, compile in release mode

\section intellisense Intellisense issues
Microsoft Visual Studio (Express) 2005 and 2008 can hang typing in a bracket or angle bracket.
This is not directly related to GGL, but caused by heavy templated libraries such as Boost and GGL.
If this is inconvenient, intellisense can easily be turned off:

<i>(...) disabling Intellisense in VC++.
There is a file called feacp.dll in &lt;VS8INSTALL&gt;/VC/vcpackages folder.
Renaming this file will disable Intellisense feature.</i>

Source: http://blogs.msdn.com/yash/archive/2007/09/19/intellisense-issues-in-visual-c-2005.aspx


*/




//---------------------------------------------------------------------------------------------------

/*!
\page performance Performance

The performance has been tested for some algorithms, concluding that Boost.Geometry is highly comparative
(http://trac.osgeo.org/ggl/wiki/Performance).

\section performance-notes Performance notes
In the page about compiling the library there are some hints provided which might improve the performance.

Furthermore it is important to realize that if you're about to do calculations with two geometries, for
example a point-in-polygon or an intersection, it is very useful to first calculate and store all bounding
boxes (envelopes), and then before doing a point-in-polygon check if the point is in the bounding box. Checking if a
point is within a box is of course much faster than visiting all vertices of a polygon.

The storage of bounding boxes is, on purpose, not done within the library because it would break the possibility
to use standard vectors of points for linestrings or rings. The library might get a traits system in the future
where geometries might tell their boundaries to the algorithms, this however would be an optional system.
*/





//---------------------------------------------------------------------------------------------------

/*!
\page DSV DSV (Delimiter Separated Values)
DSV is a text representation of a geometry, explained here: http://en.wikipedia.org/wiki/Delimiter-separated_values .
DSV can represent a point, a linestring, a polygon, or multi versions of those.

It is currently just a utility in the library, used in samples and tests, to get some textual output
of a geometry.

*/

// BSG 24-12-2010: re-added to silence Doxygen warnings:
/*!
\page WKT WKT (Well-Known Text)
WKT is a textual representation of a geometry, explained here: http://en.wikipedia.org/wiki/Well-known_text
WKT can represent a point, a linestring, a polygon, or multi versions of those.

It is currently just a utility in the library, used in samples and tests, to get some textual output
of a geometry.

*/

//---------------------------------------------------------------------------------------------------


/*!
\page OGC OGC (Open Geospatial Consortium)
OGC is the Open Geospatial Consortium, the standardization committee
on Geospatial Interoperability and GIS (Geographical Information Systems).
OGC geometries are used in many environments and databases.

The Generic Geometry Library uses OGC conventions
for algorithms and for geometry type names. Note that though OGC is concentrating on GIS,
the conventions followed here are 'geometry only', the so-called 'simple features'
(simple in the sense that polygons are not allowed to have self-intersections).

OGC specifies a library based on inheritance, as opposed to this library, which is a generic template based library
where data classes are separated from the algorithms.
Therefore this library is not an OGC implementation in the strict sense.

\section Classes
OGC defines the following geometry classes,
which are implemented as concepts (and as geometries) in the Generic Geometry Library:
- \ref boost::geometry::model::point "point": a point. The point defined here is dimensionally agnostic.
Library users does not have to use this point, they might also use their own points as long as it meets the concepts.
- \ref boost::geometry::model::linestring "linestring": Sequence of point values with linear interpolation
between points. Note that library users does not have to use this type. Algorithms works on iterators, so
all algorithms also accept iterators on a vector (or other container) of points.
- \ref boost::geometry::model::ring "linear_ring": Sequence of point values with linear interpolation
between points, which is closed and not self-intersecting
- \ref boost::geometry::model::polygon "polygon": Plane figure, consisting of an outer ring and zero or more
inner rings. So basically a polygon which might have holes.
<em>Note that this definition is different from several other polygon definitions and libraries,
where polygons are not allowed to have holes. These polygons are comparable to the linear_ring above</em>

And multi-geometries:
- \ref boost::geometry::model::multi_point "multi_point": collection of points
- \ref boost::geometry::model::multi_linestring "multi_linestring": collection of linestrings
- \ref boost::geometry::model::multi_polygon "multi_polygon": collection of polygons

The naming of these classes is used in:
- WKT (Well-Known Text)
- KML (Google Maps)
- GML
- many GIS/geometry libraries

and in many databases:
- Oracle Spatial
- SQL Server 2008
- PostGreSQL
- MySQL
- MonetDB

Besides this the Generic Geometry Library provides the following additional classes:
- \ref boost::geometry::model::box "box": Box, used for selections and for envelopes (bounding boxes)
- \ref boost::geometry::model::segment "segment": Segment, helper class, used for e.g. intersections

Finally geometry types can be added as extensions. This is e.g. done with an
"n-sphere" meaning "circle" and "sphere". Circle is convenient for selections.


\section Differences
The Generic Geometry Library does not implement the OGC Simple Feature interface exactly and completely.
There are many differences. Below the most important differences are listed.
- In OGC all operations are class methods. The Generic  Geometry Library is a template library and defines the algorithms
as generic functions.
- In OGC a point is defined by an x-coordinate value, a y-coordinate value and possibly a z-coordinate value
and a measured value. In the Generic Geometry Library the basic point defines coordinates in a neutral way,
so there is no x, no y.
- In OGC all geometries have additional members, such as SRID (spatial reference system id) or isMeasured. These
properties are not defined in the Generic Geometry Library. Library users can implement them, if necessary,
in derived classes.
- In OGC the envelope returns a geometry, in the Generic Geometry Library it returns a box
- The OGC algorithm asText is named "wkt"
- Union and intersection are currently named "X_inserter" to indicate that they
  insert there produced geometries (if any) as an OutputIterator


More information on OGC can be found on their website, http://www.opengeospatial.org
and on Wikipedia http://en.wikipedia.org/wiki/Open_Geospatial_Consortium
*/


//---------------------------------------------------------------------------------------------------

/*!
\page status Status, previews and formal review

\section introduction Introduction

Boost.Geometry is accepted by Boost (http://permalink.gmane.org/gmane.comp.lib.boost.announce/246)
after formal review (http://permalink.gmane.org/gmane.comp.lib.boost.announce/239)

The first preview was a template 2D geometry library providing its own geometries and algorithms working on
those geometries. Those geometries are still provided but not essential anymore.

The second preview didn't asume points with .x() and .y() anymore. It provided strategies,
operating on specific point types. So point_xy (cartesian points) were
handled different from point_ll (latlong points). That is still the case (now using a coordinate system meta-function)

The third preview introducted the point concept, library users could use their own points with the algorithms
provided by the library.

The fourth preview implemented concepts for all geometries. Besides that tag dispatching was
introduced internally. All algorithms were made generic.

The Formal Review continued with the design of the fourth preview, made it more consistent
(apply everywhere, all implementation within struct's, structs used as building blocks for
multi-implementations, etc). Things were made more conform Boost standards (template parameters,
no tabs, etc).

\section changes4 Changes since preview 3
- there are now typedefs for the more common geometries, such as point_2d or ring_ll_deg
- all geometries are optional, everything is working with concepts
- the convex hull algorithm has been added and can be calculated for linestrings (ranges, point sequences), polygons
- map projections (92 !) have been added, they are converted to C++ template classes from PROJ4
- transformations have been added, to go from one coordinate system to another
- conversions have been added, to go from one geometry type to another (e.g. BOX to POLYGON)
- edit functionality have been added, to edit coordinate values or to add points
- parsing of latitude longitude coordinate values have been added
- the "selected" algorithm have been added
- many examples have been added
- many tests have been added

\b Breaking \b changes
Because the library was in preview, and it still is, there are some major changes which might influence library user's
code. Although there are many changes internally, the changes for users should be relatively small:
- all algorithms accepting linestrings are now modified to get the linestring itself. In the previous version
  .begin(), end() had to be specified, This is not necessary anymore, because the Boost Range Library is used
  internally, and tag dispatching is used to distinguish different geometries
- the "grow" utility is now splitted into buffer (growing a box with a value was in fact a buffer) and a expand.
- there was a generic "get" function with a const ref and a non const ref. This is splitted into "get" and "set"
- there might be more changes, please contact if anything is unclear

\section changes5 Changes since preview 4
There are the following changes:
- implementation of spatial set relations (intersection, union)
- implementation of some spatial boolean relations (disjoint, equal, intersects, overlaps)
- move of GIS-specific code to extension/gis
- move of map projections to extensions/gis/projections
- implementation of SVG in extensions/SVG
- review of all code, conform Boost code guidelines, no tabs, template parameters, etc.
- other small changes.

\b Breaking \b changes
- there is now distinction between functions using an output iterator, and functions
  resulting in modifying geometries. Functions using an output iterator are called _inserter,
  so convex_hull_inserter inserts points (the hull) into something.
- many headerfiles are moved and/or renamed
- strategies now define a method \b apply, in previous versions this was \b operator()
  <i>this only influences your code if you've made strategies yourself</i>
- ...


\section changes6 Changes since formal review
- namespace changes
- intersection: fixed Access Violoation which could occur at wrongly oriented input
- intersection: fixed omitting outer polygon if it was equal
- intersection: performance improvement, non quadratic behaviour on monotonic sections
- intersection: now supporting GMP and CLN number types
        (there are still some comparisons causing implicit casts to double, which will be removed)
- intersection: support of input consisting of pointer-types
- small other changes (strategies transform and intersection do now have apply instead of operator() and relate)


If people are interested in helping with the library, be it by coding, by testing, by commenting or otherwise, they
are very welcome.

\section history History
Geodan started in 1995 with a Geographic Library, called geolib or also GGL (Geodan Geographic Library).
Since then hundreds of projects have
been done using this geolib, and the geolib have been extended to fit the needs of its users. Geolib
can be used in different environments: in Windows applications, as a DLL (ggl32.dll), in
Web Map Servers (SclMapServer), Web Feature Servers or more specific programs.

In 2007-2009 the Geometry part has been revised and completely templatized.
From 2008 Bruno Lalande, already a Boost contributor, joined and helped
to make the library more generic and to define clear concepts.
It was later now called Generic Geometry Library (GGL).
From 2009 Mateusz Loskot, an active member of Geospatial Open Source communities,
joined and helped with reviewing code, guidelines, WKB, iterators, and the Wiki and ticket system (http://trac.osgeo.org/ggl)

The library is now called Boost.Geometry (formerly: Generic Geometry Library, abbreviated to GGL).
Boost.Geometry is Open Source and is accepted by Boost libraries.


*/


//---------------------------------------------------------------------------------------------------

/*!
\page download Download Boost.Geometry

The library can be downloaded from Boost SVN. There is anonymous SVN access.

The command

<tt>svn co https://svn.boost.org/svn/boost/sandbox/geometry geometry</tt>

will download the library to your machine.

The library can also be downloaded from Geodan SVN. <i>This will be moved to osgeo SVN</i>

*/





#endif // _DOXYGEN_PAGES_HPP
