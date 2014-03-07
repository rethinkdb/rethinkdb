// Boost.Geometry (aka Boost.Geometry, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _DOXYGEN_ROBUSTNESS_HPP
#define _DOXYGEN_ROBUSTNESS_HPP

/---------------------------------------------------------------------------------------------------

/*!
\page robustness Boost.Geometry and Robustness

\section robustness_par1 Introduction

Floating point coordinates have limited precision. Geometry algorithms have to take this into account.

If differences between points are very small, it may lead to false result of a mathematical calculation performed on such points, what in turn, may cause algorithm result as inadequate to actual geometric situation. For example, a point which is located left from a segment, but \b very close to it, can be reported on that segment or right from it. Also if differences are a little larger, artifacts can be shown in geometric algorithms.

See for more backgrounds e.g.:

- <a href="http://www.mpi-inf.mpg.de/~kettner/pub/nonrobust_cgta_06.pdf">Classroom Examples of Robustness Problems in Geometric Computations</a>
- <a href="http://groups.csail.mit.edu/graphics/classes/6.838/S98/meetings/m12/pred/m12.html">Robust Predicates and Degeneracy</a>

Boost.Geometry is aware of these issues and provides several approaches to minimize the problems, or avoid them completely using

- <a href="http://en.wikipedia.org/wiki/GNU_Multi-Precision_Library">GMP</a>
- <a href="http://en.wikipedia.org/wiki/Class_Library_for_Numbers">CLN</a>


\section robustness_par2 Example

An example. Consider the elongated triangle and box below.

\image html robust_triangle_box.png

The left edge of the triangle has a length of about the precision of the floating point grid. It is not possible to do this intersection correctly, using floating point. No library (using floating point) can do that, by nature of float point numerical representation. It is not possible to express the four different coordinates in the resulting intersected polygon. Theoretically distinct points will be assigned to the very same location.

Also if the left edge is longer than that, or using double coordinates, those effects will be there. And of course not only in triangles, but any spiky feature in a polygon can result in non representable coordinates or zero-length edges.

\section robustness_par3 Coordinate types

All geometry types provided by Boost.Geometry, and types by the user, do have a coordinate type. For example

\code
boost::geometry::point_xy<float> p1;
boost::geometry::point_xy<double> p2;
boost::geometry::point_xy<long double> p3;
\endcode

describes three points with different coordinate types,
a 32 bits <a href="http://en.wikipedia.org/wiki/Single_precision">float</a>,
a 64 bits <a href="http://en.wikipedia.org/wiki/Double_precision_floating-point_format">double</a>,
a <a href="http://en.wikipedia.org/wiki/Long_double">long double</a>, not standardized by IEEE and is on some machines 96 bits (using a MSVC compiler it is a synonym for a double).

By default, algorithms select the coordinate type of the input geometries. If there are two input geometries, and they have different coordinate types, the coordinate type with the most precision is selected. This is done by the meta-function \b select_most_precise.

Boost.Geometry supports also high precision arithmetic types, by adaption. The numeric_adaptor, used for adaption, is not part of Boost.Geometry itself but developed by us and sent (as preview) to the Boost List (as it turned out, that functionality might also be provided by Boost.Math bindings, but the mechanism is the same). Types from the following libraries are supported:

- GMP (http://gmplib.org)
- CLN (http://www.ginac.de/CLN)

Note that the libraries themselves are not included in Boost.Geometry, they are completely independant of each other.

These numeric types can be used as following:
\code
boost::geometry::point_xy<boost::numeric_adaptor::gmp_value_type> p4;
boost::geometry::point_xy<boost::numeric_adaptor::cln_value_type> p5;
\endcode

All algorithms using these points will use the \b GMP resp. \b CLN library for calculations.

\section robustness_par4 Calculation types

If high precision arithmetic types are used as shown above, coordinates are stored in these points. That is not always necessary. Therefore, Boost.Geometry provides a second approach. It is possible to specify that only the calculation is done in high precision. This is done by specifying a strategy. For example, in area:

Example:
    The code below shows the calculation of the area. Points are stored in double; calculation is done using \b GMP

\code
    {
        typedef boost::geometry::point_xy<double> point_type;
        boost::geometry::linear_ring<point_type> ring;
        ring.push_back(boost::geometry::make<point_type>(0.0, 0.0));
        ring.push_back(boost::geometry::make<point_type>(0.0, 0.0012));
        ring.push_back(boost::geometry::make<point_type>(1234567.89012345, 0.0));
        ring.push_back(ring.front());

        typedef boost::numeric_adaptor::gmp_value_type gmp;

        gmp area = boost::geometry::area(ring, boost::geometry::strategy::area::by_triangles<point_type, gmp>());
        std::cout << area << std::endl;
    }
\endcode

Above shows how this is used to use \b GMP or \b CLN for double coordinates. Exactly the same mechanism works (of course) also to do calculation in double, where coordinates are stored in float.

\section robustness_par5 Strategies

In the previous section was shown that strategies have an optional template parameter \b CalculationType so enhance precision. However, the design of Boost.Geometry also allows that the user can implement a strategy himself. In that case he can implement the necessary predicates, or use the necessary floating point types, or even go to integer and back. Whatever he prefers.

\section robustness_par6 Examples

We show here some things which can occur in challenging domains.

The image below is drawn in PowerPoint to predict what would happen at an intersection of two triangles using float coordinates in the range 1e-45.

\image html robust_float.png

If we perform the intersection using Boost.Geometry, we get the effect that is presented on the pictures below, using float (left) and using double (right).

\image html robust_triangles.png

This shows how double can solve issues which might be present in float. However, there are also cases which cannot be solved by double or long double. And there are also cases which give more deviations than just a move of the intersection points.

We investigated this and created an artificial case. In this case, there are two stars, they are the same but one of them is rotated over an interval of about 1e-44. When those stars are intersected, the current Boost.Geometry implementation using float, double or long double will give some artifacts.

Those artifacts are caused by taking the wrong paths on points where distances are zero (because they cannot be expressed in the used coordinate systems).

If using \b GMP or \b CLN, the intersection is correct.

\image html robust_stars.png

Note again, these things happen in differences of about 1e-45. We can investigate if they can be reduced or sometimes even solved. However, they belong to the floating point approach, which is not exact.

For many users, this all is not relevant. Using double they will be able to do all operations without any problems or artefacts. They can occur in real life, where differences are very small, or very large. Those users can use \b GMP to use Boost.Geometry without any problem.

\section robustness_par7 Future work

There are several methods to avoid instability and we don't know them all, some of them might be applicable to our algorithms. Therefore is stated that Boost.Geometry is "not checked on 100% robustness". As pointed out in the discussions on the Boost mailing list in spring '09 (a.o. <i>"The dependent concept should explicitely require unlimited precision since the library specifically uses it to *guarantee* robustness. [...] Users should be left with no choice but to pick some external component fulfilling the unlimited precision requirement"</i>), it seems that it is not possible to solve all these issues using any FP number, that it is necessary to use a library as GMP or CLN for this guarantee.

Therefore we decided to go for supporting high precision numeric types first, and they are currently supported in most algorithms (a.o. area, length, perimeter, distance, centroid, intersection, union). However, we certainly are willing to take other measures as well.

\section robustness_par8 Summary

Boost.Geometry approach to support high precision:

- library users can use points with float, double, or long double coordinates (the default)
- for higher numerical robustness: users can call algorithms using another calculation type (e.g. \b GMP or \b CLN, ...)
- for higher numerical robustness: users can use points with \b GMP or \b CLN coordinates
- users can implement their own implementation and provide this as a strategy, the Boost.Geometry design allows this
- other measures can be implemented, described as future work.



*/

#endif // _DOXYGEN_ROBUSTNESS_HPP
