.. Copyright (C) 2004-2009 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

===========================================
|Logo| Connected Components Parallel Search
===========================================

::

   namespace graph { namespace distributed {
     template<typename Graph, typename ComponentMap>
     typename property_traits<ComponentMap>::value_type
     connected_components_ps(const Graph& g, ComponentMap c)
   } }

The ``connected_components_ps()`` function computes the connected
components of a graph by performing a breadth-first search from
several sources in parallel while recording and eventually resolving
the collisions.

.. contents::

Where Defined
-------------
<``boost/graph/distributed/connected_components_parallel_search.hpp``>

Parameters
----------

IN:  ``const Graph& g``
  The graph type must be a model of `Distributed Graph`_.  The graph
  type must also model the `Incidence Graph`_ and be directed.

OUT:  ``ComponentMap c``
  The algorithm computes how many connected components are in the
  graph, and assigns each component an integer label.  The algorithm
  then records to which component each vertex in the graph belongs by
  recording the component number in the component property map.  The
  ``ComponentMap`` type must be a `Distributed Property Map`_.  The
  value type must be the ``vertices_size_type`` of the graph.  The key
  type must be the graph's vertex descriptor type.

Complexity
----------

*O(PN^2 + VNP)* work, in *O(N + V)* time, where N is the
number of mappings and V is the number of local vertices.

Algorithm Description
---------------------

Every *N* th nodes starts a parallel search from the first vertex in
their local vertex list during the first superstep (the other nodes
remain idle during the first superstep to reduce the number of
conflicts in numbering the components).  At each superstep, all new
component mappings from remote nodes are handled.  If there is no work
from remote updates, a new vertex is removed from the local list and
added to the work queue.

Components are allocated from the ``component_value_allocator``
object, which ensures that a given component number is unique in the
system, currently by using the rank and number of processes to stride
allocations.

When two components are discovered to actually be the same component,
a collision is recorded.  The lower component number is prefered in
the resolution, so component numbering resolution is consistent.
After the search has exhausted all vertices in the graph, the mapping
is shared with all processes and they independently resolve the
comonent mapping.  This phase can likely be significantly sped up if a
clever algorithm for the reduction can be found.

-----------------------------------------------------------------------------

Copyright (C) 2009 The Trustees of Indiana University.

Authors: Brian Barrett, Douglas Gregor, and Andrew Lumsdaine

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _Distributed Graph: DistributedGraph.html
.. _Distributed Property Map: distributed_property_map.html
.. _Incidence Graph: http://www.boost.org/libs/graph/doc/IncidenceGraph.html
