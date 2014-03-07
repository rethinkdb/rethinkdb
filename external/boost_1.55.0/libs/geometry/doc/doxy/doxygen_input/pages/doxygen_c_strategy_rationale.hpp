// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _DOXYGEN_STRATEGY_RATIONALE_HPP
#define _DOXYGEN_STRATEGY_RATIONALE_HPP


//---------------------------------------------------------------------------------------------------

/*!
\page strategy Strategy rationale


\section strpar1 Purpose of strategies

A strategy is (Wikipedia, http://en.wikipedia.org/wiki/Strategy_pattern) a software design pattern whereby algorithms can be selected at runtime. According to Wikipedia, it is also known as the Policy pattern. In the C++ template contexts, policies are usually meant as selecting functionality at compile time. so it is reasonable to state that a strategy can also be used at compile-time

Within GGL the term policy is used (sparsely), but in a broader, or in another context. The term Strategy is used specificly for a computation method targeted to a specific coordinate system

GGL-Strategies do have the following purposes:
 - for each coordinate system, a default strategy is selected by compile time, using the coordinate system tag. This is effectively tag dispatching.
 - users can override the default choice by using the overloaded function, which has a strategy as an extra parameter, and take another strategy
 - users can override the default choice by using the overloaded function, to use the default strategy, but constructed with specific parameters
 - users can override the default choice by using the overloaded function, to use the default strategy (which can be a templated structure), with other template parameters then the default ones
 - users can override the default choice by using the overloaded function, to use the default strategy (which can be a templated structure), with other template parameters then the default ones, with the specific purpose as to select another calculation type (e.g. GMP or another big number type)

All this happens at compile-time.

For example (this is also explained in the design rationale) the distance functionality. The default distance strategy for point-point is Pythagoras (for cartesian coordinate systems) or Haversine (for spherical) or Andoyer (for geographic). Haversine works on the unit sphere, radius 1. Library users can use the distance function, specifying haversine strategy constructed with a radius of 2. Or they can use the distance function, specifying the more precise Vincenty strategy (for geographic coordinate systems, but that might not even be checked there). Specifying strategies is useful, even if not point-point distance is to be calculated, but e.g. point-polygon distance. In the end it will call the elementary specified functionality. Note that, for this example, the point-segment distance strategy is also "elementary". Note also that it can have an optional template parameter defining the underlying point-point-distance-strategy.

\section strpar2 Properties of strategies

Because strategies can be constructed outside the calling function, they can be specified as an optional parameter (implemented as an overloaded function), and not as only a template-parameter. Furthermore, strategies might be used several times, in several function calls. Therefore they are declared as const reference parameter, they should be stateless (besides construction information).

The strategy needs to access construction information (member variables), its calculation method is therefore usually not a static method but a non-static const method. It can then access member variables, while still being const, non-mutable, stateless, being able to be called across several function calls.
If often has  to keep some calculation information (state), so it should (for some algorithms) declare a state_type. In those cases, that state_type is instantiated before first call and specified in each call.
The calculation method is always called \b apply (as convention in GGL) and gets the most elementary information as a parameter: a point, a segment, a range. It depends on the algorithm and, sometimes, on the source geometry passed. That should actually be the case as least as possible
In most cases, there is an additional method \b result which returns the calculated result. That result-method is a also non-static const method, and the state is passed.
Note that the methods might be non-static const, but they might also be static. That is not checked by the concept-checker.

A strategy for a specific algorithm has a concept. The distance-strategy should follow the distance-strategy-concept. The point-in-polygon strategy should follow the point-in-polygon-strategy-concept. Those concepts are not modelled as traits classes (contrary to the geometries). The reason for this is that it seems not necessary to use legacy classes as concepts, without modification. A wrapper can be built. So the strategies should have a method \b apply and should define some types.

Which types, and which additional methods (often a method \b result), is dependant on the algorithm / type of strategy.

Strategies are checked by a strategy-concept-checker. For this checker (and sometimes for checking alone), they should define some types. Because if no types are defined, the methods cannot be checked at compile time... The strategy-concept-checkers are thus implemented per algorithm and they use the Boost Concept Check Library for checking.

So as explained above, the essence of the design is:
- function determines default-strategy, or is called with specified strategy
- function calls dispatch (dispatching is done on geometry_tag)
- dispatch calls implementation (in namespace detail), which can be shared for different geometry types and for single/multi
- implementation calls strategy (if applicable), with the smallest common (geometric) element applicable for all calculation method, the common denominator

\section strpar3 Alternative designs

Some calculations (strategies) might need to take the whole geometry, instead of working on point-per-point or segment-per-segment base. This would bypass the dispatch functionality. Because all strategies would take the whole geometry, it is not necessary to dispatch per geometry type. In fact this dispatching on geometry-type is moved to the strategy_traits class (which are specialized per coordinate system in the current design). So in this alternative design, the strategy traits class specializes on both geometry-tag and coordinate-system-tag, to select the default strategy.
For the default strategy, this move from "dispatch" to another dispatch called "strategy_XXX" (XXX is the algorithm) might make sense. However, if library users would call the overloaded function and specify a strategy, the only thing what would happen is that that specified strategy is called. So, for example:
\code
template <typename G1, typename G2, typename S>
bool within(G1 const& g1, G2 const& g2, S& const strategy)
{
  return strategy.apply(g1, g2);
}
\endcode

The library user could call just this strategy.apply(..) method directly. If more strategies are provided by the library or its extensions,
it would still make sense: the user can still call \b within and does not have to call the \b apply method of the strategy.

The convex hull strategy currently works on a whole geometry. However, it is possible that it will be reshaped to work per range (the algorithm
internally works per range), plus a mechanism to pass ranges multiple times (it currently is two-pass).


*/

#endif // _DOXYGEN_STRATEGY_RATIONALE_HPP
