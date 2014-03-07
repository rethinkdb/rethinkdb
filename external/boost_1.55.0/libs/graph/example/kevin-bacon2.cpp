//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <boost/tuple/tuple.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <map>
#include <boost/graph/adj_list_serialize.hpp>
#include <boost/archive/text_iarchive.hpp>

struct vertex_properties {
  std::string name;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & name;
  }  
};

struct edge_properties {
  std::string name;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & name;
  }  
};

using namespace boost;

typedef adjacency_list<vecS, vecS, undirectedS, 
               vertex_properties, edge_properties> Graph;
typedef graph_traits<Graph>::vertex_descriptor Vertex;
typedef graph_traits<Graph>::edge_descriptor Edge;

class bacon_number_recorder : public default_bfs_visitor
{
public:
  bacon_number_recorder(int* dist) : d(dist) { }

  void tree_edge(Edge e, const Graph& g) const {
    Vertex u = source(e, g), v = target(e, g);
    d[v] = d[u] + 1;
  }
private:
  int* d;
};

int main()
{
  std::ifstream ifs("./kevin-bacon2.dat");
  if (!ifs) {
    std::cerr << "No ./kevin-bacon2.dat file" << std::endl;
    return EXIT_FAILURE;
  }
  archive::text_iarchive ia(ifs);
  Graph g;
  ia >> g;

  std::vector<int> bacon_number(num_vertices(g));

  // Get the vertex for Kevin Bacon
  Vertex src;
  graph_traits<Graph>::vertex_iterator i, end;
  for (boost::tie(i, end) = vertices(g); i != end; ++i)
    if (g[*i].name == "Kevin Bacon")
      src = *i;

  // Set Kevin's number to zero
  bacon_number[src] = 0;

  // Perform a breadth first search to compute everyone' Bacon number.
  breadth_first_search(g, src,
                       visitor(bacon_number_recorder(&bacon_number[0])));

  for (boost::tie(i, end) = vertices(g); i != end; ++i)
    std::cout << g[*i].name << " has a Bacon number of "
          << bacon_number[*i] << std::endl;

  return 0;
}
