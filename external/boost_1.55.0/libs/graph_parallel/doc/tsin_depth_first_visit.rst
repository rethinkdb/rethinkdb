.. Copyright (C) 2004-2008 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

========================
|Logo| Depth-First Visit
========================

::

  template<typename DistributedGraph, typename DFSVisitor>
  void
  depth_first_visit(const DistributedGraph& g,
                    typename graph_traits<DistributedGraph>::vertex_descriptor s,
                    DFSVisitor vis);

  namespace graph {
    template<typename DistributedGraph, typename DFSVisitor, 
           typename VertexIndexMap>
    void
    tsin_depth_first_visit(const DistributedGraph& g,
                           typename graph_traits<DistributedGraph>::vertex_descriptor s,
                           DFSVisitor vis);

    template<typename DistributedGraph, typename DFSVisitor, 
             typename VertexIndexMap>
    void
    tsin_depth_first_visit(const DistributedGraph& g,
                           typename graph_traits<DistributedGraph>::vertex_descriptor s,
                           DFSVisitor vis, VertexIndexMap index_map);

    template<typename DistributedGraph, typename ColorMap, typename ParentMap,
             typename ExploreMap, typename NextOutEdgeMap, typename DFSVisitor>
    void
    tsin_depth_first_visit(const DistributedGraph& g,
                           typename graph_traits<DistributedGraph>::vertex_descriptor s,
                           DFSVisitor vis, ColorMap color, ParentMap parent, ExploreMap explore, 
                           NextOutEdgeMap next_out_edge);
  } 

The ``depth_first_visit()`` function performs a distributed
depth-first traversal of an undirected graph using Tsin's corrections
[Tsin02]_ to Cidon's algorithm [Cidon88]_. The distributed DFS is
syntactically similar to its `sequential counterpart`_, which provides
additional background and discussion. Differences in semantics are
highlighted here, and we refer the reader to the documentation of the
`sequential depth-first search`_ for the remainder of the
details. Visitors passed to depth-first search need to account for the
distribution of vertices across processes, because events will be
triggered on certain processes but not others. See the section
`Visitor Event Points`_ for details.

Where Defined
-------------
<``boost/graph/distributed/depth_first_search.hpp``>

also available in 

<``boost/graph/depth_first_search.hpp``>

Parameters
----------

IN: ``const Graph& g``
  The graph type must be a model of `Distributed Graph`_. The graph
  must be undirected.

IN: ``vertex_descriptor s``
  The start vertex must be the same in every process.

IN: ``DFSVisitor vis``
  The visitor must be a distributed DFS visitor. The suble differences
  between sequential and distributed DFS visitors are discussed in the
  section `Visitor Event Points`_.

IN: ``VertexIndexMap map``
  A model of `Readable Property Map`_ whose key type is the vertex
  descriptor type of the graph ``g`` and whose value type is an
  integral type. The property map should map from vertices to their
  (local) indices in the range *[0, num_vertices(g))*.

  **Default**: ``get(vertex_index, g)``

UTIL/OUT: ``ColorMap color``
  The color map must be a `Distributed Property Map`_ with the same
  process group as the graph ``g`` whose colors must monotonically
  darken (white -> gray -> black). 

  **Default**: A distributed ``iterator_property_map`` created from a
  ``std::vector`` of ``default_color_type``.

UTIL/OUT: ``ParentMap parent``
  The parent map must be a `Distributed Property Map`_ with the same
  process group as the graph ``g`` whose key and values types are the
  same as the vertex descriptor type of the graph ``g``. This
  property map holds the parent of each vertex in the depth-first
  search tree.

  **Default**: A distributed ``iterator_property_map`` created from a
  ``std::vector`` of the vertex descriptor type of the graph.

UTIL: ``ExploreMap explore``
  The explore map must be a `Distributed Property Map`_ with the same
  process group as the graph ``g`` whose key and values types are the
  same as the vertex descriptor type of the graph ``g``. 

  **Default**: A distributed ``iterator_property_map`` created from a
  ``std::vector`` of the vertex descriptor type of the graph.

UTIL: ``NextOutEdgeMap next_out_edge``
  The next out-edge map must be a `Distributed Property Map`_ with the
  same process group as the graph ``g`` whose key type is the vertex
  descriptor of the graph ``g`` and whose value type is the
  ``out_edge_iterator`` type of the graph. It is used internally to
  keep track of the next edge that should be traversed from each
  vertex.

  **Default**: A distributed ``iterator_property_map`` created from a
  ``std::vector`` of the ``out_edge_iterator`` type of the graph.

Complexity
----------
Depth-first search is inherently sequential, so parallel speedup is
very poor. Regardless of the number of processors, the algorithm will
not be faster than *O(V)*; see [Tsin02]_ for more details.

Visitor Event Points
--------------------
The `DFS Visitor`_ concept defines 8 event points that will be
triggered by the `sequential depth-first search`_. The distributed
DFS retains these event points, but the sequence of events
triggered and the process in which each event occurs will change
depending on the distribution of the graph. 

``initialize_vertex(s, g)``
  This will be invoked by every process for each local vertex.


``discover_vertex(u, g)``
  This will be invoked each time a process discovers a new vertex
  ``u``. 


``examine_vertex(u, g)``
  This will be invoked by the process owning the vertex ``u``. 

``examine_edge(e, g)``
  This will be invoked by the process owning the source vertex of
  ``e``. 


``tree_edge(e, g)``
  Similar to ``examine_edge``, this will be invoked by the process
  owning the source vertex and may be invoked only once. 


``back_edge(e, g)``
  Some edges that would be forward or cross edges in the sequential
  DFS may be detected as back edges by the distributed DFS, so extra
  ``back_edge`` events may be received.

``forward_or_cross_edge(e, g)``
  Some edges that would be forward or cross edges in the sequential
  DFS may be detected as back edges by the distributed DFS, so fewer
  ``forward_or_cross_edge`` events may be received in the distributed
  algorithm than in the sequential case.

``finish_vertex(e, g)``
  See documentation for ``examine_vertex``.

Making Visitors Safe for Distributed DFS
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The three most important things to remember when updating an existing
DFS visitor for distributed DFS or writing a new distributed DFS
visitor are:

1. Be sure that all state is either entirely local or in a
   distributed data structure (most likely a property map!) using
   the same process group as the graph.

2. Be sure that the visitor doesn't require precise event sequences
   that cannot be guaranteed by distributed DFS, e.g., requiring
   ``back_edge`` and ``forward_or_cross_edge`` events to be completely
   distinct.

3. Be sure that the visitor can operate on incomplete
   information. This often includes using an appropriate reduction
   operation in a `distributed property map`_ and verifying that
   results written are "better" that what was previously written. 

Bibliography
------------

.. [Cidon88] Isreal Cidon. Yet another distributed depth-first-search
  algorithm. Information Processing Letters, 26(6):301--305, 1988.


.. [Tsin02] Y. H. Tsin. Some remarks on distributed depth-first
  search. Information Processing Letters, 82(4):173--178, 2002.

-----------------------------------------------------------------------------

Copyright (C) 2005 The Trustees of Indiana University.

Authors: Douglas Gregor and Andrew Lumsdaine

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _sequential counterpart: http://www.boost.org/libs/graph/doc/depth_first_visit.html
.. _sequential depth-first search: http://www.boost.org/libs/graph/doc/depth_first_visit.html
.. _Distributed Graph: DistributedGraph.html
.. _Immediate Process Group: concepts/ImmediateProcessGroup.html
.. _Distributed Property Map: distributed_property_map.html
.. _DFS Visitor: http://www.boost.org/libs/graph/doc/DFSVisitor.html
.. _Readable Property Map: http://www.boost.org/libs/property_map/ReadablePropertyMap.html
