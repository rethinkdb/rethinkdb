//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//
//  Sample output:
//
//   0  --(8, 10)--> 1  
//
//   1  --(12, 20)--> 4 --(12, 40)--> 3 
//        <--(8,10)-- 0 <--(16,20)-- 2  
//   2  --(16, 20)--> 1 
//        <--(16,20)-- 5        
//   3  --(12, 40)--> 6 
//        <--(12,40)-- 1        
//   4  --(12, 20)--> 7 
//        <--(12,20)-- 1        
//   5  --(16, 20)--> 2 
//        <--(16,20)-- 6        
//   6  --(16, 20)--> 5 --(8, 10)--> 8  
//        <--(12,20)-- 7        <--(12,40)-- 3  
//   7  --(12, 20)--> 6 
//        <--(12,20)-- 4        
//   8  
//        <--(8,10)-- 6 
//
//
//   0  --(8, 1)--> 1   
//
//   1  --(12, 2)--> 4  --(12, 3)--> 3  
//        <--(8,1)-- 0  <--(16,4)-- 2   
//   2  --(16, 4)--> 1  
//        <--(16,7)-- 5 
//   3  --(12, 5)--> 6  
//        <--(12,3)-- 1 
//   4  --(12, 6)--> 7  
//        <--(12,2)-- 1 
//   5  --(16, 7)--> 2  
//        <--(16,8)-- 6 
//   6  --(16, 8)--> 5  
//        <--(12,10)-- 7        <--(12,5)-- 3   
//   7  --(12, 10)--> 6 
//        <--(12,6)-- 4 
//   8  
//
        
#include <boost/config.hpp>
#include <iostream>

#include <boost/utility.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/adjacency_list.hpp>


using namespace boost;
using namespace std;


enum edge_myflow_t { edge_myflow };
enum edge_mycapacity_t { edge_mycapacity };

namespace boost {
  BOOST_INSTALL_PROPERTY(edge, myflow);
  BOOST_INSTALL_PROPERTY(edge, mycapacity);
}


template <class Graph>
void print_network(const Graph& G)
{
  typedef typename boost::graph_traits<Graph>::vertex_iterator    Viter;
  typedef typename boost::graph_traits<Graph>::out_edge_iterator OutEdgeIter;
  typedef typename boost::graph_traits<Graph>::in_edge_iterator InEdgeIter;

  typename property_map<Graph, edge_mycapacity_t>::const_type
    capacity = get(edge_mycapacity, G);
  typename property_map<Graph, edge_myflow_t>::const_type    
    flow = get(edge_myflow, G);

  Viter ui, uiend;
  boost::tie(ui, uiend) = vertices(G);

  for (; ui != uiend; ++ui) {
    OutEdgeIter out, out_end;
    cout << *ui << "\t";

    boost::tie(out, out_end) = out_edges(*ui, G);
    for(; out != out_end; ++out)
      cout << "--(" << capacity[*out] << ", " << flow[*out] << ")--> "
           << target(*out,G) << "\t";

    InEdgeIter in, in_end;
    cout << endl << "\t";
    boost::tie(in, in_end) = in_edges(*ui, G);
    for(; in != in_end; ++in)
      cout << "<--(" << capacity[*in] << "," << flow[*in] << ")-- "
           << source(*in,G) << "\t";

    cout << endl;
  }
}


int main(int , char* [])
{
  typedef property<edge_mycapacity_t, int> Cap;
  typedef property<edge_myflow_t, int, Cap> Flow;
  typedef adjacency_list<vecS, vecS, bidirectionalS, 
     no_property, Flow> Graph;

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

  add_edge(0, 1, Flow(10, Cap(8)), G);

  add_edge(1, 4, Flow(20, Cap(12)), G);
  add_edge(4, 7, Flow(20, Cap(12)), G);
  add_edge(7, 6, Flow(20, Cap(12)), G);

  add_edge(1, 3, Flow(40, Cap(12)), G);
  add_edge(3, 6, Flow(40, Cap(12)), G);

  add_edge(6, 5, Flow(20, Cap(16)), G);
  add_edge(5, 2, Flow(20, Cap(16)), G);
  add_edge(2, 1, Flow(20, Cap(16)), G);

  add_edge(6, 8, Flow(10, Cap(8)), G);

  typedef boost::graph_traits<Graph>::edge_descriptor Edge;

  print_network(G);

  property_map<Graph, edge_myflow_t>::type
    flow = get(edge_myflow, G);

  boost::graph_traits<Graph>::vertex_iterator v, v_end;
  boost::graph_traits<Graph>::out_edge_iterator e, e_end;
  int f = 0;
  for (boost::tie(v, v_end) = vertices(G); v != v_end; ++v)
    for (boost::tie(e, e_end) = out_edges(*v, G); e != e_end; ++e)
      flow[*e] = ++f;
  cout << endl << endl;

  remove_edge(6, 8, G);

  print_network(G);

          
  return 0;
}
