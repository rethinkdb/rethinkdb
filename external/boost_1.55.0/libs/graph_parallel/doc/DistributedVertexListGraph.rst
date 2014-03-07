.. Copyright (C) 2004-2008 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

============================================
|Logo| Concept Distributed Vertex List Graph
============================================

.. contents::

Description
-----------

A Distributed Vertex List Graph is a graph whose vertices are
distributed across multiple processes or address spaces. The
``vertices`` and ``num_vertices`` functions retain the same
signatures as in the `Vertex List Graph`_ concept, but return only
the local set (and size of the local set) of vertices. 

Notation
--------

G
  A type that models the Distributed Vertex List Graph concept.

g
  An object of type ``G``.

Refinement of
-------------

  - `Graph`_

Associated types
----------------

+----------------+---------------------------------------+---------------------------------+
|Vertex          |``graph_traits<G>::vertex_descriptor`` |Must model the                   |
|descriptor type |                                       |`Global Descriptor`_ concept.    |
+----------------+---------------------------------------+---------------------------------+
|Vertex iterator |``graph_traits<G>::vertex_iterator``   |Iterates over vertices stored    |
|type            |                                       |locally. The value type must be  |
|                |                                       |``vertex_descriptor``.           |
+----------------+---------------------------------------+---------------------------------+
|Vertices size   |``graph_traits<G>::vertices_size_type``|The unsigned integral type used  |
|type            |                                       |to store the number of vertices  |
|                |                                       |in the local subgraph.           |
+----------------+---------------------------------------+---------------------------------+

Valid Expressions
-----------------

+----------------+---------------------+----------------------+-------------------------------------+
|Name            |Expression           |Type                  |Semantics                            |
+================+=====================+======================+=====================================+
|Local vertex set|``vertices(g)``      |``std::pair<``        |Returns an iterator range            |
|                |                     |``vertex_iterator,``  |providing access to the local        |
|                |                     |``vertex_iterator>``  |vertices in the graph.               |
+----------------+---------------------+----------------------+-------------------------------------+
|Number of local |``num_vertices(g)``  |``vertices_size_type``|Returns the number of vertices       |
|vertices.       |                     |                      |stored locally in the graph.         |
+----------------+---------------------+----------------------+-------------------------------------+


Models
------

  - `Distributed adjacency list`_

-----------------------------------------------------------------------------

Copyright (C) 2005 The Trustees of Indiana University.

Authors: Douglas Gregor and Andrew Lumsdaine

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _Graph: http://www.boost.org/libs/graph/doc/Graph.html
.. _Vertex List Graph: http://www.boost.org/libs/graph/doc/VertexListGraph.html
.. _Distributed Graph: DistributedGraph.html
.. _Global descriptor: GlobalDescriptor.html
.. _Distributed adjacency list: distributed_adjacency_list.html
