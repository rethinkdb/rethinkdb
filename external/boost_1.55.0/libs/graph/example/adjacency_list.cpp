//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <iostream>
#include <vector>
#include <utility>
#include <string>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/property_map/property_map.hpp>

/*
  Sample Output

  graph name: foo
  0  --joe--> 1  
  1  --joe--> 0   --curly--> 2   --dick--> 3  
  2  --curly--> 1   --tom--> 4  
  3  --dick--> 1   --harry--> 4  
  4  --tom--> 2   --harry--> 3  
  (0,1) (1,2) (1,3) (2,4) (3,4) 

  removing edge (1,3): 
  0  --joe--> 1  
  1  --joe--> 0   --curly--> 2  
  2  --curly--> 1   --tom--> 4  
  3  --harry--> 4  
  4  --tom--> 2   --harry--> 3  
  (0,1) (1,2) (2,4) (3,4) 

 */

struct EdgeProperties {
  EdgeProperties(const std::string& n) : name(n) { }
  std::string name;
};

struct VertexProperties {
  std::size_t index;
  boost::default_color_type color;
};


int main(int , char* [])
{
  using namespace boost;
  using namespace std;

  typedef adjacency_list<vecS, listS, undirectedS, 
    VertexProperties, EdgeProperties> Graph;

  const int V = 5;
  Graph g(V);

  property_map<Graph, std::size_t VertexProperties::*>::type 
    id = get(&VertexProperties::index, g);
  property_map<Graph, std::string EdgeProperties::*>::type 
    name = get(&EdgeProperties::name, g);

  boost::graph_traits<Graph>::vertex_iterator vi, viend;
  int vnum = 0;

  for (boost::tie(vi,viend) = vertices(g); vi != viend; ++vi)
    id[*vi] = vnum++;

  add_edge(vertex(0, g), vertex(1, g), EdgeProperties("joe"), g);
  add_edge(vertex(1, g), vertex(2, g), EdgeProperties("curly"), g);
  add_edge(vertex(1, g), vertex(3, g), EdgeProperties("dick"), g);
  add_edge(vertex(2, g), vertex(4, g), EdgeProperties("tom"), g);
  add_edge(vertex(3, g), vertex(4, g), EdgeProperties("harry"), g);

  graph_traits<Graph>::vertex_iterator i, end;
  graph_traits<Graph>::out_edge_iterator ei, edge_end;
  for (boost::tie(i,end) = vertices(g); i != end; ++i) {
    cout << id[*i] << " ";
    for (boost::tie(ei,edge_end) = out_edges(*i, g); ei != edge_end; ++ei)
      cout << " --" << name[*ei] << "--> " << id[target(*ei, g)] << "  ";
    cout << endl;
  }
  print_edges(g, id);

  cout << endl << "removing edge (1,3): " << endl;  
  remove_edge(vertex(1, g), vertex(3, g), g);

  ei = out_edges(vertex(1, g), g).first;
  cout << "removing edge (" << id[source(*ei, g)] 
       << "," << id[target(*ei, g)] << ")" << endl;
  remove_edge(ei, g);

  for(boost::tie(i,end) = vertices(g); i != end; ++i) {
    cout << id[*i] << " ";
    for (boost::tie(ei,edge_end) = out_edges(*i, g); ei != edge_end; ++ei)
      cout << " --" << name[*ei] << "--> " << id[target(*ei, g)] << "  ";
    cout << endl;
  }

  print_edges(g, id);

  return 0;
}
