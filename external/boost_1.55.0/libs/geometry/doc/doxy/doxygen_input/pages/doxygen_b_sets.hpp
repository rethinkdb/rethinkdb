// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _DOXYGEN_SETS_HPP
#define _DOXYGEN_SETS_HPP


//---------------------------------------------------------------------------------------------------

/*!
\page sets Spatial set-theoretic operations (union, intersection, difference)




\section sets_par1 Introduction

The GGL implementation of the algorithms intersection, union, difference and symmetric difference is based on set theory (a.o. http://en.wikipedia.org/wiki/Set_(mathematics)).
This theory is applied for spatial sets.

Intersection and union are so-called set-theoretic operations. Those operations
work on sets, and geometries (especially
polygons and multi-polygons) can be seen as sets, sets of points.


The first section will repeat a small, relevant part of the algebra of sets, also describing the notation used in this page. The next section will extend this algebra of sets for spatial sets (polygons).

\section sets_par2 Algebra of sets in a spatial context

- A &#8745; B : the intersection of two sets A and B is the set that contains all elements of A that also belong to B (aka AND)
- A &#8746; B : the union of two sets A and B is the set that contains all elements that belong to A or B (aka OR)
- A<small><sup>c</sup></small> :    the complement of set A is the set of elements which do not belong to A
- A \ B :    the difference of two sets A and B is the set of elements which belong to A but not to B
- A &#8710; B : the symmetric difference of two sets A and B is the set of elements which belong to either A or to B, but not to A and B (aka XOR)

(Source of most definitions: http://en.wikipedia.org/wiki/Algebra_of_sets)

There are several laws on sets and we will not discuss them all here. The most important for this page are:
- B \ A = A<small><sup>c</sup></small> &#8745; B and, vice versa, A \ B = B<small><sup>c</sup></small> &#8745; A
- A &#8710; B = (B \ A) &#8746; (A \ B)  (http://www.tutorvista.com/content/math/number-theory/sets/operations-sets.php)


\section sets_par3 Polygons


Polygons are sets of points, and, therefore polygons follow all definitions and laws for sets. For pragmatic reasons and implementations in computer programs, polygons have an orientation, clockwise or counter clockwise. Orientation is not part of most set theory descriptions, but is an important aspect for the appliance of sets to polygon operations.

If a polygon is (arbitrarily) defined as having its vertices in clockwise direction:
- then its interior lies on the right side of the edges [http://gandraxa.com/draw_orthogonal_polygons.aspx]
- and its exterior lies, therefore, on the left side of the edges

This definition is important for the spatial interpretation sets.

- If set A describes the interior of a polygon, then A<small><sup>c</sup></small>, its complement, describes the exterior of a polygon.
- Stated differently, set A is a polygon, all points belonging to A are inside the polygon. Its complement, A<small><sup>c</sup></small>, contains all points not belonging to A.
- If A is a polygon with its vertices oriented clockwise, A<small><sup>c</sup></small> is a polygon with the same vertices, but in reverse order, so counter clockwise. Both sets have their points belonging to them at the right side of their edges

\image html set_a_ac.png

The last observation is helpful in calculating the difference and the symmetric difference:
- the difference B \ A is defined above by the law B \ A = A<small><sup>c</sup></small> &#8745; B.
    In polygon terms it is therefore the intersection of the "reverse of A" with B.
    To calculate it, it is enough to have polygon A input in reverse order, and intersect this with polygon B
- the symmetric difference A &#8710; B is defined above by the law (B \ A) &#8746; (A \ B), which leads to
    (A<small><sup>c</sup></small> &#8745; B) &#8746; (B<small><sup>c</sup></small> &#8745; A).
    To calculate the symmetric difference, it is enough to have polygon A input in reverse order,
    intersect this with polygon B, store the result; then have polygon B input in reverse order,
    intersect this with polygon A, add this to the result and this is the symmetric difference.
    The combination of both sub-results does not have to be intersected as it is only touching
    on vertices and do not have overlaps.


\section sets_par4 Implementation of intersection and union
All spatial set-theoretic operations are implemented in shared code. There is hardly any difference in code
between the calculation of an intersection or a union. The only difference is that at each intersection point,
for an intersection the right turn should be taken. For union the left turn should be taken.

\image html set_int_right_union_left.png


This is implemented as such in GGL. The turn to be taken is a variable.

There is an alternative to calculate union as well:
- the union A &#8746; B equals to the complement of the intersection of the complements of the inputs,
    (A<small><sup>c</sup></small> &#8745; B<small><sup>c</sup></small>)<small><sup>c</sup></small> (De Morgan law,
    http://en.wikipedia.org/wiki/Algebra_of_sets#Some_additional_laws_for_complements)

There is an additional difference in the handling of disjoint holes (holes which are not intersected). This is also
implemented in the same generic way (the implementation will still be tweaked a little to have it even more generic).

For a counter clockwise polygon, the behaviour is the reverse: for intersection take the left path, for union
take the right path. This is a trivial thing to implement, but it still has to be done (as the orientation was introduced
in a later phase in GGL).


\section sets_par5 Iterating forward or reverse
As explained above, for a difference, the vertices of the first polygon should be iterated by a forward iterator, but
the vertices of the second polygon should be iterated by a reverse iterator (or vice versa). This (trivial) implementation
still has to be done. It will <b>not</b> be implemented by creating a copy, reversing it, and presenting it as input to the
set operation (as outlined above). That is easy and will work but has a performance penalty. Instead a reversible iterator will used,
extended from Boost.Range iterators, and decorating a Boost.Range iterator at the same time, which can travel forward or backward.

It is currently named \b reversible_view and usage looks like:

\code

template <int Direction, typename Range>
void walk(Range const & range)
{
    typedef reversible_view<Range, Direction> view_type;
    view_type view(range);

    typename boost::range_const_iterator<view_type>::type it;
    for (it = boost::begin(view); it != boost::end(view); ++it)
    {
        // do something
    }
}

walk<1>(range); // forward
walk<-1>(range); // backward

\endcode


\section sets_par6 Characteristics of the intersection algorithm
The algorithm is a modern variant of the graph traversal algorithm, after Weiler-Atherton
(http://en.wikipedia.org/wiki/Weiler-Atherton_clipping_algorithm).

It has the following characteristics (part of these points are deviations from Weiler-Atherton)
- No copy is necessary (the original Weiler-Atherton, and its descendants, insert intersections in (linked) lists,
    which require to make first copies of both input geometries).
- All turning points (which are intersection points with side/turn information) are unaware of the context, so we have (and need) no information about
    if, before the intersection point, a segment was inside or outside of the other geometry
- It can do intersections, unions, symmetric differences, differences
- It can handle polygons with holes, non-convex polygons, polygons-with-polygons, polygons-with-boxes (clip), rings, multi-polygons
- It can handle one polygon at the time (resolve self-intersections), two polygons (the normal use case), or more polygons (applicable
    for intersections and unions)
- It can handle clockwise and counter-clockwise geometries


\section sets_par7 Outline of the intersection algorithm
The actual implementation consists of the next phases.

\b 1 the input geometries are indexed (if necessary). Currently we use monotonic sections for the index. It is done
by the algorithm \b sectionalize. Sections are created is done on the fly, so no preparation is required before (though this would improve
performance - it is possible that there will be an alternative variant where prepared sections or other indexes are part of the input).
For box-polygon this phase is not necessary and skipped. Sectionalizing is done in linear time.

\b 2, intersection points are calculated. Segments of polygon A are compared with segments of polygon B. Segment intersection is only
done for segments in overlapping sections. Intersection points are not inserted into the original polygon or in a copy. A linked list is therefore not necessary.
This phase is called \b get_intersection_points. This function can currently be used for one or two input geometries, for self-intersection
or for intersection.
Because found intersections are provided with intersection-information, including a reference to their source, it is possible (but currently not
implemented) to have more than two geometry inputs.

The complexity of getting the intersections is (much) less than quadratic (n*m) because of the monotonic sections. The exact complexity depends on
the number of sections, of how the input polygons look like. In a worst case scenario, there are only two monotonic sections per polygon and both
overlap. The complexity is then quadratic. However, the sectionalize algorithm has a maximum number of segments per section, so for large polygons
there are always more monotonic sections and in those cases they do not overlap by the definition of "monotonic".
For boxes, the complexity is linear time.

To give another idea of how sections and indexes work:
For a test processing 3918 polygons (but not processing those of which envelopes do not overlap):
- brute force (O(n<small><sup>2</sup></small>)): 11856331 combinations
- monotonic sections: 213732  combinations (55 times less)
- a spatial index: 34877 combinations (1/6 of using monotonic sections)
So there can still be gained some performance by another index. However the current spatial index implementation (in an extension,
not in Formal Review) will still be revisited, so it is currently not used.

<i>In "normal" cases 84% of the time is spent on finding intersection points. These divisions in %'s refers to the performance test described elsewhere</i>

One piece of information per intersection points is if it is \b trivial. It is trivial if the intersection is not located at segment end points.

\b 3, the found intersection points are merged (\b merge_intersection_points), and some intersections can be deleted (e.g. in case of
collinearities). This merge process consists of sorting the intersection points in X (major) and Y (minor) direction, and merging intersections with a common location together. Intersections with common
locations do occur as soon as segments are collinear or meet at their end points.
This phase is skipped if all intersection points are trivial.

<i>About 6% is spent on merging.</i>

\b 4, some turns need to be adapted. If segments intersect in their interiors, this is never necessary. However, if segments intersect on their
end points, it is sometimes necessary to change "side" information to "turn" information. This phase is called \b adapt_turns.

The image below gives one example when adapting turns is necessary. There is side information, both segments have sides \b left and \b right, there
is also \b collinear.
However, for an intersection no turn should be taken at all, so no right turn. For a union, both polygons have to be travelled.
In this case the side information is adapted to turn information, both turns will be \b left. This phase is skipped if all intersection points are trivial.

\image html set_adapt_turns.png


\b 5, the merged intersection points are enriched (\b enrich_intersection_points) with information about a.o. the next intersection point (travel information).

<i>About 3% is spent on enrichment.</i>

\b 6, polygons are traversed (\b traverse) using the intersection points, enriched with travel information. The input polygons are traversed and
at all intersection poitns a direction is taken, left for union, right for intersection point (for counter clockwise polygons this is the other way
round). In some cases separate rings are produced. In some cases new holes are formed.

<i>About 6% is spent on traversal.</i>

\b 7, the created rings are assembled (\b assemble) into polygon(s) with exterior rings and interior rings. Even if there are no intersection points found, this process can be important to find containment and coverage.

<i>Timing of this phase is not yet available, as the comparison program work on rings.</i>


*/

#endif // _DOXYGEN_SETS_HPP
