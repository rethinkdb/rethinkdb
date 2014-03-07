//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <iostream>
#include <boost/graph/adjacency_list.hpp>
using namespace boost;

template < typename UndirectedGraph > void
undirected_graph_demo1()
{
  const int V = 3;
  UndirectedGraph undigraph(V);
  typename graph_traits < UndirectedGraph >::vertex_descriptor zero, one, two;
  typename graph_traits < UndirectedGraph >::out_edge_iterator out, out_end;
  typename graph_traits < UndirectedGraph >::in_edge_iterator in, in_end;

  zero = vertex(0, undigraph);
  one = vertex(1, undigraph);
  two = vertex(2, undigraph);
  add_edge(zero, one, undigraph);
  add_edge(zero, two, undigraph);
  add_edge(one, two, undigraph);

  std::cout << "out_edges(0):";
  for (boost::tie(out, out_end) = out_edges(zero, undigraph); out != out_end; ++out)
    std::cout << ' ' << *out;
  std::cout << std::endl << "in_edges(0):";
  for (boost::tie(in, in_end) = in_edges(zero, undigraph); in != in_end; ++in)
    std::cout << ' ' << *in;
  std::cout << std::endl;
}

template < typename DirectedGraph > void
directed_graph_demo()
{
  const int V = 2;
  DirectedGraph digraph(V);
  typename graph_traits < DirectedGraph >::vertex_descriptor u, v;
  typedef typename DirectedGraph::edge_property_type Weight;
  typename property_map < DirectedGraph, edge_weight_t >::type
    weight = get(edge_weight, digraph);
  typename graph_traits < DirectedGraph >::edge_descriptor e1, e2;
  bool found;

  u = vertex(0, digraph);
  v = vertex(1, digraph);
  add_edge(u, v, Weight(1.2), digraph);
  add_edge(v, u, Weight(2.4), digraph);
  boost::tie(e1, found) = edge(u, v, digraph);
  boost::tie(e2, found) = edge(v, u, digraph);
  std::cout << "in a directed graph is ";
#ifdef __GNUC__
  // no boolalpha
  std::cout << "(u,v) == (v,u) ? " << (e1 == e2) << std::endl;
#else
  std::cout << "(u,v) == (v,u) ? "
    << std::boolalpha << (e1 == e2) << std::endl;
#endif
  std::cout << "weight[(u,v)] = " << get(weight, e1) << std::endl;
  std::cout << "weight[(v,u)] = " << get(weight, e2) << std::endl;
}

template < typename UndirectedGraph > void
undirected_graph_demo2()
{
  const int V = 2;
  UndirectedGraph undigraph(V);
  typename graph_traits < UndirectedGraph >::vertex_descriptor u, v;
  typedef typename UndirectedGraph::edge_property_type Weight;
  typename property_map < UndirectedGraph, edge_weight_t >::type
    weight = get(edge_weight, undigraph);
  typename graph_traits < UndirectedGraph >::edge_descriptor e1, e2;
  bool found;

  u = vertex(0, undigraph);
  v = vertex(1, undigraph);
  add_edge(u, v, Weight(3.1), undigraph);
  boost::tie(e1, found) = edge(u, v, undigraph);
  boost::tie(e2, found) = edge(v, u, undigraph);
  std::cout << "in an undirected graph is ";
#ifdef __GNUC__
  std::cout << "(u,v) == (v,u) ? " << (e1 == e2) << std::endl;
#else
  std::cout << "(u,v) == (v,u) ? "
    << std::boolalpha << (e1 == e2) << std::endl;
#endif
  std::cout << "weight[(u,v)] = " << get(weight, e1) << std::endl;
  std::cout << "weight[(v,u)] = " << get(weight, e2) << std::endl;

  std::cout << "the edges incident to v: ";
  typename boost::graph_traits<UndirectedGraph>::out_edge_iterator e, e_end;
  typename boost::graph_traits<UndirectedGraph>::vertex_descriptor 
    s = vertex(0, undigraph);
  for (boost::tie(e, e_end) = out_edges(s, undigraph); e != e_end; ++e)
    std::cout << "(" << source(*e, undigraph) 
              << "," << target(*e, undigraph) << ")" << std::endl;
}


int
main()
{
  typedef property < edge_weight_t, double >Weight;
  typedef adjacency_list < vecS, vecS, undirectedS,
    no_property, Weight > UndirectedGraph;
  typedef adjacency_list < vecS, vecS, directedS,
    no_property, Weight > DirectedGraph;
  undirected_graph_demo1 < UndirectedGraph > ();
  directed_graph_demo < DirectedGraph > ();
  undirected_graph_demo2 < UndirectedGraph > ();
  return 0;
}
