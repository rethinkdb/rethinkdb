.. Copyright (C) 2004-2008 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

==========================================
|Logo| Concept Distributed Edge List Graph
==========================================

.. contents::

Description
-----------

A Distributed Edge List Graph is a graph whose vertices are
distributed across multiple processes or address spaces. The
``vertices`` and ``num_vertices`` functions retain the same
signatures as in the `Edge List Graph`_ concept, but return only
the local set (and size of the local set) of vertices. 

Notation
--------

G
  A type that models the Distributed Edge List Graph concept.

g
  An object of type ``G``.

Refinement of
-------------

  - `Graph`_

Associated types
----------------

+----------------+---------------------------------------+---------------------------------+
|Edge            |``graph_traits<G>::edge_descriptor``   |Must model the                   |
|descriptor type |                                       |`Global Descriptor`_ concept.    |
+----------------+---------------------------------------+---------------------------------+
|Edge iterator   |``graph_traits<G>::edge_iterator``     |Iterates over edges stored       |
|type            |                                       |locally. The value type must be  |
|                |                                       |``edge_descriptor``.             |
+----------------+---------------------------------------+---------------------------------+
|Edges size      |``graph_traits<G>::edges_size_type``   |The unsigned integral type used  |
|type            |                                       |to store the number of edges     |
|                |                                       |in the local subgraph.           |
+----------------+---------------------------------------+---------------------------------+

Valid Expressions
-----------------

+----------------+---------------------+----------------------+-------------------------------------+
|Name            |Expression           |Type                  |Semantics                            |
+================+=====================+======================+=====================================+
|Local edge set  |``edges(g)``         |``std::pair<``        |Returns an iterator range            |
|                |                     |``edge_iterator,``    |providing access to the local        |
|                |                     |``edge_iterator>``    |edges in the graph.                  |
+----------------+---------------------+----------------------+-------------------------------------+
|Number of local |``num_edges(g)``     |``edges_size_type``   |Returns the number of edges          |
|edges.          |                     |                      |stored locally in the graph.         |
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
.. _Edge List Graph: http://www.boost.org/libs/graph/doc/EdgeListGraph.html
.. _Distributed Graph: DistributedGraph.html
.. _Global descriptor: GlobalDescriptor.html
.. _Distributed adjacency list: distributed_adjacency_list.html
