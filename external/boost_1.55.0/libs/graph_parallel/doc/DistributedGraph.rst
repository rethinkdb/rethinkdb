.. Copyright (C) 2004-2008 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

================================
|Logo| Concept Distributed Graph
================================

.. contents::

Description
-----------

A Distributed Graph is a graph whose vertices or edges are
distributed across multiple processes or address spaces. The
descriptors of a Distributed Graph must model the `Global
Descriptor`_ concept. 

Notation
--------

G
  A type that models the Distributed Graph concept.


Refinement of
-------------

  - Graph_

Associated types
----------------

+----------------+---------------------------------------+---------------------------------+
|Vertex          |``graph_traits<G>::vertex_descriptor`` |Must model the                   |
|descriptor type |                                       |`Global Descriptor`_ concept.    |
+----------------+---------------------------------------+---------------------------------+
|Edge            |``graph_traits<G>::edge_descriptor``   |Must model the                   |
|descriptor type |                                       |`Global Descriptor`_ concept.    |
+----------------+---------------------------------------+---------------------------------+


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
.. _Global descriptor: GlobalDescriptor.html
.. _Distributed adjacency list: distributed_adjacency_list.html
