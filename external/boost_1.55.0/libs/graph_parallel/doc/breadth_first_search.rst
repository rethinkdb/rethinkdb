.. Copyright (C) 2004-2008 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

===========================
|Logo| Breadth-First Search
===========================

::

  // named parameter version
  template <class Graph, class P, class T, class R>
  void breadth_first_search(Graph& G, 
    typename graph_traits<Graph>::vertex_descriptor s, 
    const bgl_named_params<P, T, R>& params);

  // non-named parameter version
  template <class Graph, class Buffer, class BFSVisitor, 
            class ColorMap>
  void breadth_first_search(const Graph& g, 
     typename graph_traits<Graph>::vertex_descriptor s, 
     Buffer& Q, BFSVisitor vis, ColorMap color);

The ``breadth_first_search()`` function performs a distributed breadth-first
traversal of a directed or undirected graph. The distributed BFS is
syntactically equivalent to its `sequential counterpart`_, which
provides additional background and discussion. Differences in
semantics are highlighted here, and we refer the reader to the
documentation of the `sequential breadth-first search`_ for the
remainder of the details.

This distributed breadth-first search algorithm implements a
*level-synchronized* breadth-first search, meaning that all vertices
in a given level of the BFS tree will be processed (potentially in
parallel) before any vertices from a successive level in the tree are
processed. Distributed breadth-first search visitors should account
for this behavior, a topic discussed further in `Visitor Event
Points`_. 

.. contents::

Where Defined
-------------
<``boost/graph/breadth_first_search.hpp``>

Parameter Defaults
------------------
All parameters of the `sequential breadth-first search`_ are supported
and have essentially the same meaning. Only differences are documented
here.

IN: ``Graph& g``
  The graph type must be a model of `Distributed Graph`_. 


IN: ``vertex_descriptor s``
  The start vertex must be the same in every process.


IN: ``visitor(BFSVisitor vis)``
  The visitor must be a distributed BFS visitor. The suble differences
  between sequential and distributed BFS visitors are discussed in the
  section `Visitor Event Points`_.

UTIL/OUT: ``color_map(ColorMap color)``
  The color map must be a `Distributed Property Map`_ with the same
  process group as the graph ``g`` whose colors must monotonically
  darken (white -> gray -> black). The default value is a distributed
  ``iterator_property_map`` created from a ``std::vector`` of
  ``default_color_type``. 


UTIL: ``buffer(Buffer& Q)``
  The queue must be a distributed queue that passes vertices to their
  owning process. If already-visited vertices should not be visited
  again (as is typical for BFS), the queue must filter duplicates
  itself. The queue controls synchronization within the algorithm: its
  ``empty()`` method must not return ``true`` until all local queues
  are empty. 
  
  **Default:** A ``distributed_queue`` of a ``filtered_queue`` over a
    standard ``boost::queue``. This queue filters out duplicate
    vertices and distributes vertices appropriately.

Complexity
----------
This algorithm performs *O(V + E)* work in *d + 1* BSP supersteps,
where *d* is the diameter of the connected component being
searched. Over all supersteps, *O(E)* messages of constant size will
be transmitted.

Visitor Event Points
--------------------
The `BFS Visitor`_ concept defines 9 event points that will be
triggered by the `sequential breadth-first search`_. The distributed
BFS retains these nine event points, but the sequence of events
triggered and the process in which each event occurs will change
depending on the distribution of the graph. 

``initialize_vertex(s, g)``
  This will be invoked by every process for each local vertex.


``discover_vertex(u, g)``
  This will be invoked each time a process discovers a new vertex
  ``u``. Due to incomplete information in distributed property maps,
  this event may be triggered many times for the same vertex ``u``.


``examine_vertex(u, g)``
  This will be invoked by the process owning the vertex ``u``. If the
  distributed queue prevents duplicates, it will be invoked only
  once for a particular vertex ``u``.


``examine_edge(e, g)``
  This will be invoked by the process owning the source vertex of
  ``e``. If the distributed queue prevents duplicates, it will be
  invoked only once for a particular edge ``e``.


``tree_edge(e, g)``
  Similar to ``examine_edge``, this will be invoked by the process
  owning the source vertex and may be invoked only once. Unlike the
  sequential BFS, this event may be triggered even when the target has
  already been discovered (but by a different process). Thus, some
  ``non_tree_edge`` events in a sequential BFS may become
  ``tree_edge`` in a distributed BFS.


``non_tree_edge(e, g)``
  Some ``non_tree_edge`` events in a sequential BFS may become
  ``tree_edge`` events in a distributed BFS. See the description of
  ``tree_edge`` for additional details.


``gray_target(e, g)``
  As with ``tree_edge`` not knowing when another process has already
  discovered a vertex, ``gray_target`` events may occur in a
  distributed BFS when ``black_target`` events may occur in a
  sequential BFS, due to a lack of information on a given
  processor. The source of edge ``e`` will be local to the process
  executing this event.


``black_target(e, g)``
  See documentation for ``gray_target``


``finish_vertex(e, g)``
  See documentation for ``examine_vertex``.

Making Visitors Safe for Distributed BFS
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The three most important things to remember when updating an existing
BFS visitor for distributed BFS or writing a new distributed BFS
visitor are:

1. Be sure that all state is either entirely local or in a
   distributed data structure (most likely a property map!) using
   the same process group as the graph.

2. Be sure that the visitor doesn't require precise event sequences
   that cannot be guaranteed by distributed BFS, e.g., requiring
   ``tree_edge`` and ``non_tree_edge`` events to be completely
   distinct.

3. Be sure that the visitor can operate on incomplete
   information. This often includes using an appropriate reduction
   operation in a `distributed property map`_ and verifying that
   results written are "better" that what was previously written. 

Distributed BFS Visitor Example
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
To illustrate the differences between sequential and distributed BFS
visitors, we consider a BFS visitor that places the distance from the
source vertex to every other vertex in a property map. The sequential
visitor is very simple::

  template<typename DistanceMap>
  struct bfs_discovery_visitor : bfs_visitor<> 
  {
    bfs_discovery_visitor(DistanceMap distance) : distance(distance) {}

    template<typename Edge, typename Graph>
    void tree_edge(Edge e, const Graph& g)
    {
      std::size_t new_distance = get(distance, source(e, g)) + 1;
      put(distance, target(e, g), new_distance);
    }
    
   private:
    DistanceMap distance;
  };

To revisit this code for distributed BFS, we consider the three points
in the section `Making Visitors Safe for Distributed BFS`_:

1. The distance map will need to become distributed, because the
   distance to each vertex should be stored in the process owning the
   vertex. This is actually a requirement on the user to provide such
   a distributed property map, although in many cases the property map
   will automatically be distributed and no syntactic changes will be
   required. 

2. This visitor *does* require a precise sequence of events that may
   change with a distributed BFS. The extraneous ``tree_edge`` events
   that may occur could result in attempts to put distances into the
   distance map multiple times for a single vertex. We therefore need
   to consider bullet #3.

3. Since multiple distance values may be written for each vertex, we
   must always choose the best value we can find to update the
   distance map. The distributed property map ``distance`` needs to
   resolve distances to the smallest distance it has seen. For
   instance, process 0 may find vertex ``u`` at level 3 but process 1
   finds it at level 5: the distance must remain at 3. To do this, we
   state that the property map's *role* is as a distance map, which
   introduces an appropriate reduction operation::

          set_property_map_role(vertex_distance, distance);

The resulting distributed BFS visitor (which also applies, with no
changes, in the sequential BFS) is very similar to our original
sequential BFS visitor. Note the single-line difference in the
constructor::

  template<typename DistanceMap>
  struct bfs_discovery_visitor : bfs_visitor<> 
  {
    bfs_discovery_visitor(DistanceMap distance) : distance(distance) 
    {
      set_property_map_role(vertex_distance, distance);
    }

    template<typename Edge, typename Graph>
    void tree_edge(Edge e, const Graph& g)
    {
      std::size_t new_distance = get(distance, source(e, g)) + 1;
      put(distance, target(e, g), new_distance);
    }
    
   private:
    DistanceMap distance;
  };


Performance
-----------
The performance of Breadth-First Search is illustrated by the
following charts. Scaling and performance is reasonable for all of the
graphs we have tried. 

.. image:: http://www.osl.iu.edu/research/pbgl/performance/chart.php?generator=ER,SF,SW&dataset=TimeSparse&columns=4
  :align: left
.. image:: http://www.osl.iu.edu/research/pbgl/performance/chart.php?generator=ER,SF,SW&dataset=TimeSparse&columns=4&speedup=1

.. image:: http://www.osl.iu.edu/research/pbgl/performance/chart.php?generator=ER,SF,SW&dataset=TimeDense&columns=4
  :align: left
.. image:: http://www.osl.iu.edu/research/pbgl/performance/chart.php?generator=ER,SF,SW&dataset=TimeDense&columns=4&speedup=1

-----------------------------------------------------------------------------

Copyright (C) 2004 The Trustees of Indiana University.

Authors: Douglas Gregor and Andrew Lumsdaine

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _sequential counterpart: http://www.boost.org/libs/graph/doc/breadth_first_search.html
.. _sequential breadth-first search: http://www.boost.org/libs/graph/doc/breadth_first_search.html
.. _Distributed Graph: DistributedGraph.html
.. _Distributed Property Map: distributed_property_map.html
.. _BFS Visitor: http://www.boost.org/libs/graph/doc/BFSVisitor.html
