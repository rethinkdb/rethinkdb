// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _DOXYGEN_MAINPAGE_HPP
#define _DOXYGEN_MAINPAGE_HPP


// -> introduction.qbk
// -> quickstart.qbk

/*!
\mainpage Boost.Geometry

\section header Boost.Geometry (aka GGL, Generic Geometry Library)

<em>
Copyright &copy; 1995-2012 <b>Barend Gehrels</b>, Geodan, Amsterdam, the Netherlands.\n
Copyright &copy; 2008-2012 <b>Bruno Lalande</b>, Paris, France.\n
Copyright &copy; 2010-2012 <b>Mateusz Loskot</b>, Cadcorp, London, UK.\n
Distributed under the Boost Software License, Version 1.0.\n
(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
</em>

\section intro Introduction
Boost.Geometry, formally accepted by Boost, defines concepts "concepts" for geometries and
implements some algorithms on such geometries. Before acceptance by Boost it was known as GGL
(Generic Geometry Library) and this documentation still contains that name on various places.


Boost.Geometry contains a dimension-agnostic, coordinate-system-agnostic and
scalable kernel, based on concepts, meta-functions and tag- dispatching. On top
of that kernel, algorithms are built: area, length, perimeter, centroid, convex
hull, intersection (clipping), within (point in polygon), distance, envelope
(bounding box), simplify, transform, convert, and more. The library is also
designed to support high precision arithmetic numbers, such as GMP.

Boost.Geometry contains instantiable geometry classes, but library users can also use their own. Using registration macros or traits classes their geometries can be adapted to fulfil the Boost.Geometry Concepts.

Boost.Geometry might be used in all domains where geometry plays a role: mapping and GIS, gaming, computer graphics and widgets, robotics, astronomy... The core is designed to be as generic as possible and support those domains. However, for now the development has been mostly GIS-oriented.

Boost.Geometry supports the extension model, the same way as GIL also applies it. An extension is (mostly) something more specific to domains like mentioned above.

The library follows existing conventions:
- conventions from boost
- conventions from the std library
- conventions and names from one of the OGC standards on Geometry

The library can be downloaded from the Boost Sandbox,
go to the \ref download "Download page" for more information.

A (recently started) Wiki is here: http://trac.osgeo.org/ggl/wiki

\section quickstart Quick start
It is not possible to show the whole library at a glance. A few very small examples are shown below.

It should be possible to use a very small part of the library,
for example only the distance between two points.

\dontinclude doxygen_2.cpp
\skip example_for_main_page()
\skipline int a
\until endl;

Other often used algorithms are point-in-polygon:
\skipline ring_2d
\until endl;

or area:
\skip area
\until endl;

It is possible, by the nature of a template library, to mix the point types declared above:
\skip double d2
\until endl;

The pieces above generate this output:
\image html output_main.png


It is also possible to use non-Cartesian points.
For example: points on a sphere. When then an algorithm such as distance
is used the library "inspects" that it is handling spherical
points and calculates the distance over the sphere, instead of applying the Pythagorean theorem.

Finally an example from a totally different domain: developing window-based applications, for example
using QtWidgets. We check if two rectangles overlap and if so,
move the second one to another place:
\skip QRect
\until }

More examples are on the page \b Examples


*/





#endif // _DOXYGEN_MAINPAGE_HPP
