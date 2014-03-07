//  (C) Copyright Jeremy Siek 2004 
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// From Louis Lavery <Louis@devilsChimney.co.uk>
/*Expected Output:-
A:   0 A
B:  11 A

Actual Output:-
A:   0 A
B: 2147483647 B
*/

#include <iostream>
#include <iomanip>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/bellman_ford_shortest_paths.hpp>
#include <boost/cstdlib.hpp>
#include <boost/test/minimal.hpp>

int test_main(int, char*[])
{
  using namespace boost;

  enum { A, B, Z };
  char const name[] = "ABZ";
  int const numVertex = static_cast<int>(Z) + 1;
  typedef std::pair<int,int> Edge;
  Edge edge_array[] = {Edge(B,A)};
  int const numEdges = sizeof(edge_array) / sizeof(Edge);
  int const weight[numEdges] = {11};

  typedef adjacency_list<vecS,vecS,undirectedS,
    no_property,property<edge_weight_t,int> > Graph;

  Graph g(edge_array, edge_array + numEdges, numVertex);

  Graph::edge_iterator ei, ei_end;
  property_map<Graph,edge_weight_t>::type
    weight_pmap = get(edge_weight, g);

  int i = 0;
  for(boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei, ++i)
    weight_pmap[*ei] = weight[i];

  std::vector<int> parent(numVertex);
  for(i = 0; i < numVertex; ++i)
    parent[i] = i;

  int inf = (std::numeric_limits<int>::max)();
  std::vector<int> distance(numVertex, inf);
  distance[A] = 0; // Set source distance to zero

  bool const r = bellman_ford_shortest_paths
    (g, int (numVertex),
     weight_pmap, 
     boost::make_iterator_property_map(parent.begin(), get(boost::vertex_index, g)),
     boost::make_iterator_property_map(distance.begin(), get(boost::vertex_index, g)),
     closed_plus<int>(),
     std::less<int>(),
     default_bellman_visitor());

  if (r) {
    for(int i = 0; i < numVertex; ++i) {
      std::cout << name[i] << ": ";
      if (distance[i] == inf)
        std::cout  << std::setw(3) << "inf";
      else
        std::cout << std::setw(3) << distance[i];
      std::cout << " " << name[parent[i]] << std::endl;
    }
  } else {
    std::cout << "negative cycle" << std::endl;
  }

#if !(defined(__INTEL_COMPILER) && __INTEL_COMPILER <= 700) && !(defined(BOOST_MSVC) && BOOST_MSVC <= 1300)
  graph_traits<Graph>::vertex_descriptor s = vertex(A, g);
  std::vector<int> parent2(numVertex);
  std::vector<int> distance2(numVertex, 17);
  bool const r2 = bellman_ford_shortest_paths
                    (g, 
                     weight_map(weight_pmap).
                     distance_map(boost::make_iterator_property_map(distance2.begin(), get(boost::vertex_index, g))).
                     predecessor_map(boost::make_iterator_property_map(parent2.begin(), get(boost::vertex_index, g))).
                     root_vertex(s));
  if (r2) {
    for(int i = 0; i < numVertex; ++i) {
      std::cout << name[i] << ": ";
      if (distance2[i] == inf)
        std::cout  << std::setw(3) << "inf";
      else
        std::cout << std::setw(3) << distance2[i];
      std::cout << " " << name[parent2[i]] << std::endl;
    }
  } else {
    std::cout << "negative cycle" << std::endl;
  }

  BOOST_CHECK(r == r2);
  if (r && r2) {
    BOOST_CHECK(parent == parent2);
    BOOST_CHECK(distance == distance2);
  }
#endif

  return boost::exit_success;
}
