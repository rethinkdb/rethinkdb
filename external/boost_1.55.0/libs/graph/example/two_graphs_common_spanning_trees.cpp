//          Copyright (C) 2012, Michele Caini.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//          Two Graphs Common Spanning Trees Algorithm
//      Based on academic article of Mint, Read and Tarjan
//     Efficient Algorithm for Common Spanning Tree Problem
// Electron. Lett., 28 April 1983, Volume 19, Issue 9, p.346-347


#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/two_graphs_common_spanning_trees.hpp>
#include <exception>
#include <vector>


using namespace std;

typedef
boost::adjacency_list
  <
    boost::vecS,         // OutEdgeList
    boost::vecS,         // VertexList
    boost::undirectedS,  // Directed
    boost::no_property,  // VertexProperties
    boost::no_property,  // EdgeProperties
    boost::no_property,  // GraphProperties
    boost::listS         // EdgeList
  >
Graph
;

typedef
boost::graph_traits<Graph>::vertex_descriptor
vertex_descriptor;

typedef
boost::graph_traits<Graph>::edge_descriptor
edge_descriptor;

typedef
boost::graph_traits<Graph>::vertex_iterator
vertex_iterator;

typedef
boost::graph_traits<Graph>::edge_iterator
edge_iterator;


int main(int argc, char **argv)
{
  Graph iG, vG;
  vector< edge_descriptor > iG_o;
  vector< edge_descriptor > vG_o;

  iG_o.push_back(boost::add_edge(0, 1, iG).first);
  iG_o.push_back(boost::add_edge(0, 2, iG).first);
  iG_o.push_back(boost::add_edge(0, 3, iG).first);
  iG_o.push_back(boost::add_edge(0, 4, iG).first);
  iG_o.push_back(boost::add_edge(1, 2, iG).first);
  iG_o.push_back(boost::add_edge(3, 4, iG).first);

  vG_o.push_back(boost::add_edge(1, 2, vG).first);
  vG_o.push_back(boost::add_edge(2, 0, vG).first);
  vG_o.push_back(boost::add_edge(2, 3, vG).first);
  vG_o.push_back(boost::add_edge(4, 3, vG).first);
  vG_o.push_back(boost::add_edge(0, 3, vG).first);
  vG_o.push_back(boost::add_edge(0, 4, vG).first);

  vector<bool> inL(iG_o.size(), false);

  std::vector< std::vector<bool> > coll;
  boost::tree_collector<
      std::vector< std::vector<bool> >,
      std::vector<bool>
    > tree_collector(coll);
  boost::two_graphs_common_spanning_trees
    (
      iG,
      iG_o,
      vG,
      vG_o,
      tree_collector,
      inL
    );
  
  std::vector< std::vector<bool> >::iterator it;
  for(it = coll.begin(); it != coll.end(); ++it) {
    // Here you can play with the trees that the algorithm has found.
  }

  return 0;
}
