.. Copyright (C) 2004-2009 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

===================================
|Logo| Parallel Boost Graph Library
===================================

Overview
--------

The Parallel Boost Graph Library is an extension to the `Boost Graph
Library`_ (BGL) for parallel and distributed computing. It offers
distributed graphs and graph algorithms to exploit coarse-grained
parallelism along with parallel algorithms that exploit fine-grained
parallelism, while retaining the same interfaces as the (sequential)
BGL. Code written using the sequential BGL should be easy to
parallelize with the parallel BGL. Visitors new to the Parallel BGL
should read our `architectural overview`_.

1. `Process groups`_

  - `MPI process group`_
  - `Simple trigger interface`_

2. Auxiliary data structures

  - `Distributed queue`_
  - `Distributed property map`_
 
3. Distributed graph concepts

  - `Distributed Graph`_
  - `Distributed Vertex List Graph`_
  - `Distributed Edge List Graph`_
  - `Global Descriptor`_

4. Graph data structures

  - `Distributed adjacency list`_

5. Graph adaptors

  - `Local subgraph adaptor`_
  - `Vertex list graph adaptor`_

6. Graph input/output

  - Graphviz output
  - `METIS input`_

7. Synthetic data generators

  - `R-MAT generator`_
  - `Sorted R-MAT generator`_
  - `Sorted unique R-MAT generator`_
  - `Unique R-MAT generator`_
  - `Scalable R-MAT generator`_
  - `Erdos-Renyi generator`_
  - `Sorted Erdos-Renyi generator`_
  - `Small world generator`_
  - `SSCA generator`_
  - `Mesh generator`_

8. Algorithms

  - Distributed algorithms 

    - `Breadth-first search`_
    - `Dijkstra's single-source shortest paths`_

      - `Eager Dijkstra shortest paths`_
      - `Crauser et al. Dijkstra shortest paths`_
      - `Delta-Stepping shortest paths`_

    - `Depth-first search`_
    - `Minimum spanning tree`_

      - `Boruvka's minimum spanning tree`_
      - `Merging local minimum spanning forests`_
      - `Boruvka-then-merge`_
      - `Boruvka-mixed-merge`_

    - Connected components

      - `Connected components`_
      - `Connected components parallel search`_
      - `Strongly-connected components`_
    
    - PageRank_
    - `Boman et al. Graph coloring`_
    - `Fruchterman Reingold force-directed layout`_
    - `s-t connectivity`_
    - `Betweenness centrality`_
    - `Non-distributed betweenness centrality`_

----------------------------------------------------------------------------

Copyright (C) 2005-2009 The Trustees of Indiana University.

Authors: Nick Edmonds, Douglas Gregor, and Andrew Lumsdaine

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _Parallel Dijkstra example: dijkstra_example.html
.. _Boost Graph Library: http://www.boost.org/libs/graph/doc
.. _adjacency_list: http://www.boost.org/libs/graph/doc/adjacency_list.html
.. _local subgraph adaptor: local_subgraph.html
.. _vertex list graph adaptor: vertex_list_adaptor.html
.. _MPI: http://www-unix.mcs.anl.gov/mpi/
.. _generic programming: http://www.cs.rpi.edu/~musser/gp/
.. _development page: design/index.html
.. _process groups: process_group.html
.. _MPI process group: process_group.html
.. _Simple trigger interface: simple_trigger.html
.. _Open Systems Laboratory: http://www.osl.iu.edu
.. _Indiana University: http://www.indiana.edu
.. _Distributed adjacency list: distributed_adjacency_list.html
.. _Distributed queue: distributed_queue.html
.. _Distributed property map: distributed_property_map.html
.. _R-MAT generator: rmat_generator.html
.. _Sorted R-MAT generator: sorted_rmat_generator.html
.. _Sorted Unique R-MAT generator: sorted_unique_rmat_generator.html
.. _Unique R-MAT generator: unique_rmat_generator.html
.. _Scalable R-MAT generator: scalable_rmat_generator.html
.. _Erdos-Renyi generator: http://www.boost.org/libs/graph/doc/erdos_renyi_generator.html
.. _Sorted Erdos-Renyi generator: http://www.boost.org/libs/graph/doc/sorted_erdos_renyi_gen.html
.. _Small world generator: http://www.boost.org/libs/graph/doc/small_world_generator.html
.. _SSCA generator: ssca_generator.html
.. _Mesh generator: mesh_generator.html
.. _Breadth-first search: breadth_first_search.html
.. _Depth-first search: tsin_depth_first_visit.html
.. _Dijkstra's single-source shortest paths: dijkstra_shortest_paths.html
.. _Eager Dijkstra shortest paths: dijkstra_shortest_paths.html#eager-dijkstra-s-algorithm
.. _Crauser et al. Dijkstra shortest paths: dijkstra_shortest_paths.html#crauser-et-al-s-algorithm
.. _Delta-Stepping shortest paths: dijkstra_shortest_paths.html#delta-stepping-algorithm
.. _Minimum spanning tree: dehne_gotz_min_spanning_tree.html
.. _Boruvka's minimum spanning tree: dehne_gotz_min_spanning_tree.html#dense-boruvka-minimum-spanning-tree
.. _Merging local minimum spanning forests: dehne_gotz_min_spanning_tree.html#merge-local-minimum-spanning-trees
.. _Boruvka-then-merge: dehne_gotz_min_spanning_tree.html#boruvka-then-merge
.. _Boruvka-mixed-merge: dehne_gotz_min_spanning_tree.html#boruvka-mixed-merge
.. _PageRank: page_rank.html
.. _Boman et al. Graph coloring: boman_et_al_graph_coloring.html
.. _Connected components: connected_components.html
.. _Connected components parallel search: connected_components_parallel_search.html
.. _Strongly-connected components: strong_components.html
.. _Distributed Graph: DistributedGraph.html
.. _Distributed Vertex List Graph: DistributedVertexListGraph.html
.. _Distributed Edge List Graph: DistributedEdgeListGraph.html
.. _Global Descriptor: GlobalDescriptor.html
.. _METIS Input: metis.html
.. _architectural overview: overview.html
.. _Fruchterman Reingold force-directed layout: fruchterman_reingold.html
.. _s-t connectivity: st_connected.html
.. _Betweenness centrality: betweenness_centrality.html
.. _Non-distributed betweenness centrality: non_distributed_betweenness_centrality.html
