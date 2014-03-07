.. Copyright (C) 2004-2008 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

===============
|Logo| PageRank
===============

::
  
  namespace graph { 
    template<typename Graph, typename RankMap, typename Done>
    inline void
    page_rank(const Graph& g, RankMap rank_map, Done done, 
              typename property_traits<RankMap>::value_type damping = 0.85);

    template<typename Graph, typename RankMap>
    inline void
    page_rank(const Graph& g, RankMap rank_map);
  } 
  
The ``page_rank`` algorithm computes the ranking of vertices in a
graph, based on the connectivity of a directed graph [PBMW98]_. The
idea of PageRank is based on a random model of a Web surfer, who
starts a random web page and then either follows a link from that web
page (choosing from the links randomly) or jumps to a completely
different web page (not necessarily linked from the current
page). The PageRank of each page is the probability of the random web
surfer visiting that page.

.. contents::

Where Defined
~~~~~~~~~~~~~
<``boost/graph/distributed/page_rank.hpp``>

also accessible from

<``boost/graph/page_rank.hpp``>

Parameters
~~~~~~~~~~

IN: ``Graph& g``
  The graph type must be a model of `Distributed Vertex List Graph`_ and
  `Distributed Edge List Graph`_. The graph must be directed.

OUT: ``RankMap rank``
  Stores the rank of each vertex. The type ``RankMap`` must model the
  `Read/Write Property Map`_ concept and must be a `distributed
  property map`_. Its key type must be the vertex descriptor of the
  graph type and its value type must be a floating-point or rational
  type. 

IN: ``Done done``
  A function object that determines when the PageRank algorithm
  should complete. It will be passed two parameters, the rank map and
  a reference to the graph, and should return ``true`` when the
  algorithm should terminate.

  **Default**: ``graph::n_iterations(20)``

IN: ``typename property_traits<RankMap>::value_type damping``
  The damping factor is the probability that the Web surfer will
  select an outgoing link from the current page instead of jumping to
  a random page. 

  **Default**: 0.85

Complexity
~~~~~~~~~~
Each iteration of PageRank requires *O((V + E)/p)* time on *p*
processors and performs *O(V)* communication. The number of
iterations is dependent on the input to the algorithm.

Bibliography
------------

.. [PBMW98] Lawrence Page, Sergey Brin, Rajeev Motwani, and Terry
  Winograd. The PageRank Citation Ranking: Bringing Order to the
  Web. Technical report, Stanford Digital Library Technologies Project,
  November 1998.

-----------------------------------------------------------------------------

Copyright (C) 2005 The Trustees of Indiana University.

Authors: Douglas Gregor and Andrew Lumsdaine

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _Distributed Vertex List Graph: DistributedVertexListGraph.html
.. _Distributed Edge List Graph: DistributedEdgeListGraph.html
.. _Distributed property map: distributed_property_map.html
.. _Readable Property Map: http://www.boost.org/libs/property_map/ReadablePropertyMap.html
.. _Read/Write Property Map: http://www.boost.org/libs/property_map/ReadWritePropertyMap.html

