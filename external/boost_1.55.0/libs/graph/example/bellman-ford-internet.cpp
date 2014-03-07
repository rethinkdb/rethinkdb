//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <iostream>
#include <boost/graph/edge_list.hpp>
#include <boost/graph/bellman_ford_shortest_paths.hpp>

int
main()
{
  using namespace boost;
  // ID numbers for the routers (vertices).
  enum
  { A, B, C, D, E, F, G, H, n_vertices };
  const int n_edges = 11;
  typedef std::pair < int, int >Edge;

  // The list of connections between routers stored in an array.
  Edge edges[] = {
  Edge(A, B), Edge(A, C),
        Edge(B, D), Edge(B, E), Edge(C, E), Edge(C, F), Edge(D, H),
        Edge(D, E), Edge(E, H), Edge(F, G), Edge(G, H)
  };

  // Specify the graph type and declare a graph object
  typedef edge_list < Edge*, Edge, std::ptrdiff_t, std::random_access_iterator_tag> Graph;
  Graph g(edges, edges + n_edges);

  // The transmission delay values for each edge.  
  float delay[] =
    {5.0, 1.0, 1.3, 3.0, 10.0, 2.0, 6.3, 0.4, 1.3, 1.2, 0.5};

  // Declare some storage for some "external" vertex properties.
  char name[] = "ABCDEFGH";
  int parent[n_vertices];
  for (int i = 0; i < n_vertices; ++i)
    parent[i] = i;
  float distance[n_vertices];
  std::fill(distance, distance + n_vertices, (std::numeric_limits < float >::max)());
  // Specify A as the source vertex
  distance[A] = 0;

  bool r = bellman_ford_shortest_paths(g, int (n_vertices),
                                       weight_map(make_iterator_property_map
                                                  (&delay[0],
                                                   get(edge_index, g),
                                                   delay[0])).
                                       distance_map(&distance[0]).
                                       predecessor_map(&parent[0]));

  if (r)
    for (int i = 0; i < n_vertices; ++i)
      std::cout << name[i] << ": " << distance[i]
        << " " << name[parent[i]] << std::endl;
  else
    std::cout << "negative cycle" << std::endl;

  return EXIT_SUCCESS;
}
