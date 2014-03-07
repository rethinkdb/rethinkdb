.. Copyright (C) 2004-2008 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

================================
|Logo| Vertex List Graph Adaptor
================================

::

  template<typename Graph, typename GlobalIndexMap>
  class vertex_list_adaptor
  {
  public:
    vertex_list_adaptor(const Graph& g, 
                        const GlobalIndexMap& index_map = GlobalIndexMap());
  };

  template<typename Graph, typename GlobalIndexMap>
  vertex_list_adaptor<Graph, GlobalIndexMap>
  make_vertex_list_adaptor(const Graph& g, const GlobalIndexMap& index_map);

  template<typename Graph>
  vertex_list_adaptor<Graph, *unspecified*>
  make_vertex_list_adaptor(const Graph& g);


The vertex list graph adaptor adapts any model of `Distributed Vertex List
Graph`_ in a `Vertex List Graph`_. In the former type of graph, the
set of vertices is distributed across the process group, so no
process has access to all vertices. In the latter type of graph,
however, every process has access to every vertex in the graph. This
is required by some distributed algorithms, such as the
implementations of `Minimum spanning tree`_ algorithms.

.. contents::

Where Defined
-------------
<``boost/graph/distributed/vertex_list_adaptor.hpp``>


Class template ``vertex_list_adaptor``
--------------------------------------

The ``vertex_list_adaptor`` class template takes a `Distributed
Vertex List Graph`_ and a mapping from vertex descriptors to global
vertex indices, which must be in the range *[0, n)*, where *n* is the
number of vertices in the entire graph. The mapping is a `Readable
Property Map`_ whose key type is a vertex descriptor.

The vertex list adaptor stores only a reference to the underlying
graph, forwarding all operations not related to the vertex list on to
the underlying graph. For instance, if the underlying graph models
`Adjacency Graph`_, then the adaptor will also model `Adjacency
Graph`_. Note, however, that no modifications to the underlying graph
can occur through the vertex list adaptor. Modifications made to the
underlying graph directly will be reflected in the vertex list
adaptor, but modifications that add or remove vertices invalidate the
vertex list adaptor. Additionally, the vertex list adaptor provides
access to the global index map via the ``vertex_index`` property.

On construction, the vertex list adaptor performs an all-gather
operation to create a list of all vertices in the graph within each
process. It is this list that is accessed via *vertices* and the
length of this list that is accessed via *num_vertices*. Due to the
all-gather operation, the creation of this adaptor is a collective
operation. 

Function templates ``make_vertex_list_adaptor``
-----------------------------------------------

These function templates construct a vertex list adaptor from a graph
and, optionally, a property map that maps vertices to global index
numbers. 

Parameters
~~~~~~~~~~

IN: ``Graph& g``
  The graph type must be a model of `Distributed Vertex List Graph`_.

IN: ``GlobalIndexMap index_map``
  A `Distributed property map`_ whose type must model `Readable
  property map`_ that maps from vertices to a global index. If
  provided, this map must be initialized prior to be passed to the
  vertex list adaptor.

  **Default:** A property map of unspecified type constructed from a
  distributed ``iterator_property_map`` that uses the
  ``vertex_index`` property map of the underlying graph and a vector
  of ``vertices_size_type``.

Complexity
~~~~~~~~~~
These operations require *O(n)* time, where *n* is the number of
vertices in the graph, and *O(n)* communication per node in the BSP model.

-----------------------------------------------------------------------------

Copyright (C) 2004 The Trustees of Indiana University.

Authors: Douglas Gregor and Andrew Lumsdaine

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _Kruskal's algorithm: http://www.boost.org/libs/graph/doc/kruskal_min_spanning_tree.html
.. _Vertex list graph: http://www.boost.org/libs/graph/doc/VertexListGraph.html
.. _Adjacency graph: http://www.boost.org/libs/graph/doc/AdjacencyGraph.html
.. _distributed adjacency list: distributed_adjacency_list.html
.. _Minimum spanning tree: dehne_gotz_min_spanning_tree.html
.. _Distributed Vertex List Graph: DistributedVertexListGraph.html
.. _Distributed property map: distributed_property_map.html
.. _Readable Property Map: http://www.boost.org/libs/property_map/ReadablePropertyMap.html
.. _Read/Write Property Map: http://www.boost.org/libs/property_map/ReadWritePropertyMap.html


