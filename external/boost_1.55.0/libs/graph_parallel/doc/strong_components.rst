.. Copyright (C) 2004-2008 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

===========================
|Logo| Connected Components
===========================

::
  
  template<typename Graph, typename ComponentMap>
  inline typename property_traits<ComponentMap>::value_type
  strong_components( const Graph& g, ComponentMap c);

  namespace graph {
    template<typename Graph, typename VertexComponentMap>
    void
    fleischer_hendrickson_pinar_strong_components(const Graph& g, VertexComponentMap r);

    template<typename Graph, typename ReverseGraph, 
             typename ComponentMap, typename IsoMapFR, typename IsoMapRF>
    inline typename property_traits<ComponentMap>::value_type
    fleischer_hendrickson_pinar_strong_components(const Graph& g, 
                                                  ComponentMap c,
                                                  const ReverseGraph& gr, 
                                                  IsoMapFR fr, IsoMapRF rf);
  }
    
The ``strong_components()`` function computes the strongly connected
components of a directed graph.  The distributed strong components
algorithm uses the `sequential strong components`_ algorithm to
identify components local to a processor.  The distributed portion of
the algorithm is built on the `distributed breadth first search`_
algorithm and is based on the work of Fleischer, Hendrickson, and
Pinar [FHP00]_. The interface is a superset of the interface to the
BGL `sequential strong components`_ algorithm. The number of
strongly-connected components in the graph is returned to all
processes. 

The distributed strong components algorithm works on both ``directed``
and ``bidirectional`` graphs.  In the bidirectional case, a reverse
graph adapter is used to produce the required reverse graph.  In 
the directed case, a separate graph is constructed in which all the
edges are reversed.

.. contents::

Where Defined
-------------
<``boost/graph/distributed/strong_components.hpp``>

also accessible from

<``boost/graph/strong_components.hpp``>

Parameters
----------

IN:  ``const Graph& g``
  The graph type must be a model of `Distributed Graph`_.  The graph
  type must also model the `Incidence Graph`_ and be directed.

OUT:  ``ComponentMap c``
  The algorithm computes how many strongly connected components are in the
  graph, and assigns each component an integer label.  The algorithm
  then records to which component each vertex in the graph belongs by
  recording the component number in the component property map.  The
  ``ComponentMap`` type must be a `Distributed Property Map`_.  The
  value type must be the ``vertices_size_type`` of the graph.  The key
  type must be the graph's vertex descriptor type.

UTIL:  ``VertexComponentMap r``
  The algorithm computes a mapping from each vertex to the
  representative of the strong component, stored in this property map.
  The ``VertexComponentMap`` type must be a `Distributed Property Map`_.
  The value and key types must be the vertex descriptor of the graph.

IN: ``const ReverseGraph& gr``
  The reverse (or transpose) graph of ``g``, such that for each
  directed edge *(u, v)* in ``g`` there exists a directed edge
  *(fr(v), fr(u))* in ``gr`` and for each edge *(v', u')* in *gr*
  there exists an edge *(rf(u'), rf(v'))* in ``g``. The functions
  *fr* and *rf* map from vertices in the graph to the reverse graph
  and vice-verse, and are represented as property map arguments. The
  concept requirements on this graph type are equivalent to those on
  the ``Graph`` type, but the types need not be the same.

  **Default**: Either a ``reverse_graph`` adaptor over the original
  graph (if the graph type is bidirectional, i.e., models the
  `Bidirectional Graph`_ concept) or a `distributed adjacency list`_
  constructed from the input graph.

IN: ``IsoMapFR fr``
  A property map that maps from vertices in the input graph ``g`` to
  vertices in the reversed graph ``gr``. The type ``IsoMapFR`` must
  model the `Readable Property Map`_ concept and have the graph's
  vertex descriptor as its key type and the reverse graph's vertex
  descriptor as its value type.

  **Default**: An identity property map (if the graph type is
  bidirectional) or a distributed ``iterator_property_map`` (if the
  graph type is directed).

IN: ``IsoMapRF rf``
  A property map that maps from vertices in the reversed graph ``gr``
  to vertices in the input graph ``g``. The type ``IsoMapRF`` must
  model the `Readable Property Map`_ concept and have the reverse
  graph's vertex descriptor as its key type and the graph's vertex
  descriptor as its value type.

  **Default**: An identity property map (if the graph type is
  bidirectional) or a distributed ``iterator_property_map`` (if the
  graph type is directed).

Complexity
----------

The local phase of the algorithm is *O(V + E)*.  The parallel phase of
the algorithm requires at most *O(V)* supersteps each containing two 
breadth first searches which are *O(V + E)* each. 


Algorithm Description
---------------------

Prior to executing the sequential phase of the algorithm, each process
identifies any completely local strong components which it labels and
removes from the vertex set considered in the parallel phase of the
algorithm.

The parallel phase of the distributed strong components algorithm
consists of series of supersteps.  Each superstep starts with one
or more vertex sets which are guaranteed to completely contain
any remaining strong components.  A `distributed breadth first search`_
is performed starting from the first vertex in each vertex set.
All of these breadth first searches are performed in parallel by having
each processor call ``breadth_first_search()`` with a different starting
vertex, and if necessary inserting additional vertices into the 
``distributed queue`` used for breadth first search before invoking
the algorithm.  A second `distributed breadth first search`_ is
performed on the reverse graph in the same fashion.  For each initial
vertex set, the successor set (the vertices reached in the forward 
breadth first search), and the predecessor set (the vertices reached
in the backward breadth first search) is computed.  The intersection
of the predecessor and successor sets form a strongly connected 
component which is labeled as such.  The remaining vertices in the 
initial vertex set are partitioned into three subsets each guaranteed
to completely contain any remaining strong components.  These three sets
are the vertices in the predecessor set not contained in the identified
strongly connected component, the vertices in the successor set not 
in the strongly connected component, and the remaing vertices in the 
initial vertex set not in the predecessor or successor sets.  Once
new vertex sets are identified, the algorithm begins a new superstep.
The algorithm halts when no vertices remain.

To boost performance in sparse graphs when identifying small components,
when less than a given portion of the initial number of vertices 
remain in active vertex sets, a filtered graph adapter is used
to limit the graph seen by the breadth first search algorithm to the
active vertices.

Bibliography
------------

.. [FHP00] Lisa Fleischer, Bruce Hendrickson, and Ali Pinar. On
  Identifying Strongly Connected Components in Parallel. In Parallel and
  Distributed Processing (IPDPS), volume 1800 of Lecture Notes in
  Computer Science, pages 505--511, 2000. Springer.

-----------------------------------------------------------------------------

Copyright (C) 2004, 2005 The Trustees of Indiana University.

Authors: Nick Edmonds, Douglas Gregor, and Andrew Lumsdaine

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _Sequential strong components: http://www.boost.org/libs/graph/doc/strong_components.html
.. _Distributed breadth first search: breadth_first_search.html
.. _Distributed Graph: DistributedGraph.html
.. _Distributed Property Map: distributed_property_map.html
.. _Incidence Graph: http://www.boost.org/libs/graph/doc/IncidenceGraph.html
.. _Bidirectional Graph: http://www.boost.org/libs/graph/doc/BidirectionalGraph.html
.. _distributed adjacency list: distributed_adjacency_list.html
.. _Readable Property Map: http://www.boost.org/libs/property_map/ReadablePropertyMap.html
.. 
