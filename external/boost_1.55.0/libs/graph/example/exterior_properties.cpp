//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//
// This example is similar to the one in edge_property.cpp.
// The only difference is that this example uses exterior
// property storage instead of interior (properties).
//
//  Sample output:
//
//    0        --(10, 8)--> 1        
//
//    1        --(20, 12)--> 4        --(40, 12)--> 3        
//            <--(10,8)-- 0        <--(20,16)-- 2        
//    2        --(20, 16)--> 1        
//            <--(20,16)-- 5        
//    3        --(40, 12)--> 6        
//            <--(40,12)-- 1        
//    4        --(20, 12)--> 7        
//            <--(20,12)-- 1        
//    5        --(20, 16)--> 2        
//            <--(20,16)-- 6        
//    6        --(20, 16)--> 5         --(10, 8)--> 8        
//            <--(20,12)-- 7 <--(40,12)-- 3        
//    7        --(20, 12)--> 6        
//            <--(20,12)-- 4        
//    8        
//            <--(10,8)-- 6        


#include <boost/config.hpp>
#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/property_map/property_map.hpp>

template <class Graph, class Capacity, class Flow>
void print_network(Graph& G, Capacity capacity, Flow flow)
{
  typedef typename boost::graph_traits<Graph>::vertex_iterator    Viter;
  typedef typename boost::graph_traits<Graph>::out_edge_iterator OutEdgeIter;
  typedef typename boost::graph_traits<Graph>::in_edge_iterator InEdgeIter;

  Viter ui, uiend;
  for (boost::tie(ui, uiend) = boost::vertices(G); ui != uiend; ++ui) {
    OutEdgeIter out, out_end;
    std::cout << *ui << "\t";

    for(boost::tie(out, out_end) = boost::out_edges(*ui, G); out != out_end; ++out)
      std::cout << "--(" << boost::get(capacity, *out) << ", " 
           << boost::get(flow, *out) << ")--> " << boost::target(*out,G) << "\t";
    std::cout << std::endl << "\t";

    InEdgeIter in, in_end;    
    for(boost::tie(in, in_end) = boost::in_edges(*ui, G); in != in_end; ++in)
      std::cout << "<--(" << boost::get(capacity, *in) << "," << boost::get(flow, *in) << ")-- "
           << boost::source(*in, G) << "\t";
    std::cout << std::endl;
  }
}


int main(int , char* []) {

  typedef boost::adjacency_list<boost::vecS, boost::vecS, 
    boost::bidirectionalS, boost::no_property, 
    boost::property<boost::edge_index_t, std::size_t> > Graph;

  const int num_vertices = 9;
  Graph G(num_vertices);

  /*          2<----5 
             /       ^
            /         \
           V           \ 
    0 ---->1---->3----->6--->8
           \           ^
            \         /
             V       /
             4----->7
   */

  int capacity[] = { 10, 20, 20, 20, 40, 40, 20, 20, 20, 10 };
  int flow[] = { 8, 12, 12, 12, 12, 12, 16, 16, 16, 8 };

  // insert edges into the graph, and assign each edge an ID number
  // to index into the property arrays
  boost::add_edge(0, 1, 0, G);

  boost::add_edge(1, 4, 1, G);
  boost::add_edge(4, 7, 2, G);
  boost::add_edge(7, 6, 3, G);

  boost::add_edge(1, 3, 4, G);
  boost::add_edge(3, 6, 5, G);

  boost::add_edge(6, 5, 6, G);
  boost::add_edge(5, 2, 7, G);
  boost::add_edge(2, 1, 8, G);

  boost::add_edge(6, 8, 9, G);

  typedef boost::property_map<Graph, boost::edge_index_t>::type EdgeIndexMap;
  EdgeIndexMap edge_id = boost::get(boost::edge_index, G);

  typedef boost::iterator_property_map<int*, EdgeIndexMap, int, int&> IterMap;

  print_network(G, IterMap(capacity, edge_id), IterMap(flow, edge_id));
          
  return 0;
}
