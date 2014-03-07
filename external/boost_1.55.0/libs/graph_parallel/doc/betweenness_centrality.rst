.. Copyright (C) 2004-2009 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

=============================
|Logo| Betweenness Centrality
=============================

::

  // named parameter versions
  template<typename Graph, typename Param, typename Tag, typename Rest>
  void 
  brandes_betweenness_centrality(const Graph& g, 
                                 const bgl_named_params<Param,Tag,Rest>& params);

  template<typename Graph, typename CentralityMap>
  void 
  brandes_betweenness_centrality(const Graph& g, CentralityMap centrality);

  template<typename Graph, typename CentralityMap, typename EdgeCentralityMap>
  void 
  brandes_betweenness_centrality(const Graph& g, CentralityMap centrality,
                                 EdgeCentralityMap edge_centrality_map);

  // non-named parameter versions
  template<typename Graph, typename CentralityMap, typename EdgeCentralityMap,
           typename IncomingMap, typename DistanceMap, typename DependencyMap, 
           typename PathCountMap, typename VertexIndexMap, typename Buffer>
  void  
  brandes_betweenness_centrality(const Graph& g, 
                                 CentralityMap centrality,
                                 EdgeCentralityMap edge_centrality_map,
                                 IncomingMap incoming, 
                                 DistanceMap distance, 
                                 DependencyMap dependency,     
                                 PathCountMap path_count,   
                                 VertexIndexMap vertex_index,
                                 Buffer sources,
                                 typename property_traits<DistanceMap>::value_type delta);

  template<typename Graph, typename CentralityMap, typename EdgeCentralityMap, 
           typename IncomingMap, typename DistanceMap, typename DependencyMap, 
           typename PathCountMap, typename VertexIndexMap, typename WeightMap, 
           typename Buffer>    
  void 
  brandes_betweenness_centrality(const Graph& g, 
                                 CentralityMap centrality,
                                 EdgeCentralityMap edge_centrality_map,
                                 IncomingMap incoming, 
                                 DistanceMap distance, 
                                 DependencyMap dependency,
                                 PathCountMap path_count, 
                                 VertexIndexMap vertex_index,
                                 Buffer sources,
                                 typename property_traits<WeightMap>::value_type delta,
                                 WeightMap weight_map);

  // helper functions
  template<typename Graph, typename CentralityMap>
  typename property_traits<CentralityMap>::value_type
  central_point_dominance(const Graph& g, CentralityMap centrality);

The ``brandes_betweenness_centrality()`` function computes the
betweenness centrality of the vertices and edges in a graph.  The
method of calculating betweenness centrality in *O(V)* space is due to
Brandes [Brandes01]_.  The algorithm itself is a modification of
Brandes algorithm by Edmonds [Edmonds09]_.

.. contents::

Where Defined
-------------
<``boost/graph/distributed/betweenness_centrality.hpp``>

also accessible from

<``boost/graph/betweenness_centrality.hpp``>

Parameters
----------

IN:  ``const Graph& g``
  The graph type must be a model of `Distributed Graph`_.  The graph
  type must also model the `Incidence Graph`_ concept.  0-weighted
  edges in ``g`` will result in undefined behavior.

IN: ``CentralityMap centrality`` 
  A centrality map may be supplied to the algorithm, if not supplied a
  ``dummy_property_map`` will be used and no vertex centrality
  information will be recorded.  The ``CentralityMap`` type must be a
  `Distributed Property Map`_.  The key type must be the graph's
  vertex descriptor type.

  **Default**: A ``dummy_property_map``.

IN:  ``EdgeCentralityMap edge_centrality_map``
  An edge centrality map may be supplied to the algorithm, if not
  supplied a ``dummy_property_map`` will be used and no edge
  centrality information will be recorded.  The ``EdgeCentralityMap``
  type must be a `Distributed Property Map`_.  The key type must be
  the graph's vertex descriptor type.

  **Default**: A ``dummy_property_map``.

IN:  ``IncomingMap incoming``
  The incoming map contains the incoming edges to a vertex that are
  part of shortest paths to that vertex.  The ``IncomingMap`` type
  must be a `Distributed Property Map`_.  Its key type and value type
  must both be the graph's vertex descriptor type.

  **Default**: An ``iterator_property_map`` created from a
    ``std::vector`` of ``std::vector`` of the graph's vertex
    descriptor type.

IN:  ``DistanceMap distance``
  The distance map records the distance to vertices during the
  shortest paths portion of the algorithm.  The ``DistanceMap`` type
  must be a `Distributed Property Map`_.  Its key type must be the
  graph's vertex descriptor type.

  **Default**: An ``iterator_property_map`` created from a
    ``std::vector`` of the value type of the ``CentralityMap``.

IN: ``DependencyMap dependency`` 
  The dependency map records the dependency of each vertex during the
  centrality calculation portion of the algorithm.  The
  ``DependencyMap`` type must be a `Distributed Property Map`_.  Its
  key type must be the graph's vertex descriptor type.

  **Default**: An ``iterator_property_map`` created from a
    ``std::vector`` of the value type of the ``CentralityMap``.

IN:  ``PathCountMap path_count``

  The path count map records the number of shortest paths each vertex
  is on during the centrality calculation portion of the algorithm.
  The ``PathCountMap`` type must be a `Distributed Property Map`_.
  Its key type must be the graph's vertex descriptor type.

  **Default**: An ``iterator_property_map`` created from a
    ``std::vector`` of the graph's degree size type.

IN:  ``VertexIndexMap vertex_index``
  A model of `Readable Property Map`_ whose key type is the vertex
  descriptor type of the graph ``g`` and whose value type is an
  integral type. The property map should map from vertices to their
  (local) indices in the range *[0, num_vertices(g))*.

  **Default**: ``get(vertex_index, g)``

IN:  ``WeightMap weight_map``
  A model of `Readable Property Map`_ whose key type is the edge
  descriptor type of the graph ``g``.  If not supplied the betweenness
  centrality calculation will be unweighted.

IN: ``Buffer sources`` 
  A model of Buffer_ containing the starting vertices for the
  algorithm.  If ``sources`` is empty a complete betweenness
  centrality calculation using all vertices in ``g`` will be
  performed.  The value type of the Buffer must be the graph's vertex
  descriptor type.

  **Default**: An empty ``boost::queue`` of int.

Complexity
----------

Computing the shortest paths, counting them, and computing the
contribution to the centrality map is *O(V log V)*.  Calculating exact
betweenness centrality requires counting the shortest paths from all
vertices in ``g``, thus exact betweenness centrality is *O(V^2 log
V)*.

Algorithm Description
---------------------

For the vertices in ``sources`` (or all vertices in ``g`` when
``sources`` is empty) the algorithm first calls a customized
implementation of delta_stepping_shortest_paths_ which maintains a
shortest path tree using an ``IncomingMap``.  The ``IncomingMap``
contains the source of all incoming edges on shortest paths.

The ``IncomingMap`` defines the shortest path DAG at the target of the
edges in the shortest paths tree.  In the bidirectional case edge
flags could be used to translate the shortest paths information to the
source of the edges.  Setting edge flags during the shortest path
computation rather than using an ``IncomingMap`` would result in
adding an *O(V)* factor to the inner loop of the shortest paths
computation to account for having to clear edge flags when a new
shortest path is found.  This would increase the complexity of the
algorithm.  Asymptotically, the current implementation is better,
however using edge flags in the bidirectional case would reduce the
number of supersteps required by the depth of the shortest paths DAG
for each vertex.  Currently an ``outgoing`` map is explicitly
constructed by simply reversing the edges in the incoming map.  Once
the ``outgoing`` map is constructed it is traversed in dependency
order from the source of the shortest paths calculation in order to
compute path counts.  Once path counts are computed the shortest paths
DAG is again traversed in dependency order from the source to
calculate the dependency and centrality of each vertex.

The algorithm is complete when the centrality has been computed from
all vertices in ``g``.

Bibliography
------------

.. [Brandes01] Ulrik Brandes.  A Faster Algorithm for Betweenness
  Centrality. In the Journal of Mathematical Sociology, volume 25
  number 2, pages 163--177, 2001.

.. [Edmonds09] Nick Edmonds, Torsten Hoefler, and Andrew Lumsdaine.
  A Space-Efficient Parallel Algorithm for Computing Betweenness
  Centrality in Sparse Networks.  Indiana University tech report.
  2009.

-----------------------------------------------------------------------------

Copyright (C) 2009 The Trustees of Indiana University.

Authors: Nick Edmonds and Andrew Lumsdaine

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _delta_stepping_shortest_paths: dijkstra_shortest_paths.html
.. _Distributed Graph: DistributedGraph.html
.. _Incidence Graph: http://www.boost.org/libs/graph/doc/IncidenceGraph.html
.. _Readable Property Map: http://www.boost.org/libs/property_map/ReadablePropertyMap.html
.. _Buffer: http://www.boost.org/libs/graph/doc/Buffer.html
.. _Process Group: process_group.html
.. _Distributed Property Map: distributed_property_map.html
