.. Copyright (C) 2004-2008 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

=============================
|Logo| Local Subgraph Adaptor
=============================

The local subgraph adaptor takes an existing `Distributed Graph` and
filters out all of the nonlocal edges and vertices, presenting only
the local portion of the distributed graph to the user. The behavior
is equivalent to (and implemented with) a `filtered graph`_, and is a
noncopying view into the graph itself. Changes made through the
filtered graph will be reflected in the original graph and vice-versa.

::

  template<typename DistributedGraph> class local_subgraph;

  template<typename DistributedGraph>
  local_subgraph<DistributedGraph> make_local_subgraph(DistributedGraph& g);

Where Defined
-------------
<boost/graph/distributed/local_subgraph.hpp>

Reference
---------
The local subgraph adaptor adapts and forwards all operations of
distributed graphs, the signatures of which will be omitted. Only
operations unique to the local subgraph adaptor are presented.

Member Functions
~~~~~~~~~~~~~~~~

::

  local_subgraph(DistributedGraph& g);

Constructs a local subgraph presenting the local portion of the
distributed graph ``g``.

--------------------------------------------------------------------------

::
 
  DistributedGraph&         base()               { return g; }
  const DistributedGraph&   base() const         { return g; }

Returns the underlying distributed graph.

Free Functions
~~~~~~~~~~~~~~

::

  template<typename DistributedGraph>
  local_subgraph<DistributedGraph> make_local_subgraph(DistributedGraph& g);

Constructs a local subgraph presenting the local portion of the
distributed graph ``g``.

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _filtered graph: http://www.boost.org/libs/graph/doc/filtered_graph.html
