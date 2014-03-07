// -*- c++ -*-
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

#include <boost/graph/adjacency_list.hpp>

/*
  Thanks to Dale Gerdemann for this example, which inspired some
  changes to adjacency_list to make this work properly.
 */

/*
  Sample output:

  0  --c--> 1   --j--> 1   --c--> 2   --x--> 2  
  1  --c--> 2   --d--> 3  
  2  --t--> 4  
  3  --h--> 4  
  4 

  merging vertex 1 into vertex 0

  0  --c--> 0   --j--> 0   --c--> 1   --x--> 1   --d--> 2  
  1  --t--> 3  
  2  --h--> 3  
  3 
 */

// merge_vertex(u,v,g):
// incoming/outgoing edges for v become incoming/outgoing edges for u
// v is deleted
template <class Graph, class GetEdgeProperties>
void merge_vertex
  (typename boost::graph_traits<Graph>::vertex_descriptor u,
   typename boost::graph_traits<Graph>::vertex_descriptor v,
   Graph& g, GetEdgeProperties getp)
{
  typedef boost::graph_traits<Graph> Traits;
  typename Traits::edge_descriptor e;
  typename Traits::out_edge_iterator out_i, out_end;
  for (boost::tie(out_i, out_end) = out_edges(v, g); out_i != out_end; ++out_i) {
    e = *out_i;
    typename Traits::vertex_descriptor targ = target(e, g);
    add_edge(u, targ, getp(e), g);
  }
  typename Traits::in_edge_iterator in_i, in_end;
  for (boost::tie(in_i, in_end) = in_edges(v, g); in_i != in_end; ++in_i) {
    e = *in_i;
    typename Traits::vertex_descriptor src = source(e, g);
    add_edge(src, u, getp(e), g);
  }
  clear_vertex(v, g);
  remove_vertex(v, g);
}

template <class StoredEdge>
struct order_by_name
  : public std::binary_function<StoredEdge,StoredEdge,bool> 
{
  bool operator()(const StoredEdge& e1, const StoredEdge& e2) const {
    // Using std::pair operator< as an easy way to get lexicographical
    // compare over tuples.
    return std::make_pair(e1.get_target(), boost::get(boost::edge_name, e1))
      < std::make_pair(e2.get_target(), boost::get(boost::edge_name, e2));
  }
};
struct ordered_set_by_nameS { };

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
namespace boost {
  template <class ValueType>
  struct container_gen<ordered_set_by_nameS, ValueType> {
    typedef std::set<ValueType, order_by_name<ValueType> > type;
  };
  template <>
  struct parallel_edge_traits<ordered_set_by_nameS> { 
    typedef allow_parallel_edge_tag type;
  };
}
#endif

template <class Graph>
struct get_edge_name {
  get_edge_name(const Graph& g_) : g(g_) { }

  template <class Edge>
  boost::property<boost::edge_name_t, char> operator()(Edge e) const {
    return boost::property<boost::edge_name_t, char>(boost::get(boost::edge_name, g, e));
  }
  const Graph& g;
};

int
main()
{
#ifdef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
  std::cout << "This program requires partial specialization." << std::endl;
#else
  using namespace boost;
  typedef property<edge_name_t, char> EdgeProperty;
  typedef adjacency_list<ordered_set_by_nameS, vecS, bidirectionalS,
    no_property, EdgeProperty> graph_type;

  graph_type g;

  add_edge(0, 1, EdgeProperty('j'), g);
  add_edge(0, 2, EdgeProperty('c'), g);
  add_edge(0, 2, EdgeProperty('x'), g);
  add_edge(1, 3, EdgeProperty('d'), g);
  add_edge(1, 2, EdgeProperty('c'), g);
  add_edge(1, 3, EdgeProperty('d'), g);
  add_edge(2, 4, EdgeProperty('t'), g);
  add_edge(3, 4, EdgeProperty('h'), g);
  add_edge(0, 1, EdgeProperty('c'), g);
  
  property_map<graph_type, vertex_index_t>::type id = get(vertex_index, g);
  property_map<graph_type, edge_name_t>::type name = get(edge_name, g);

  graph_traits<graph_type>::vertex_iterator i, end;
  graph_traits<graph_type>::out_edge_iterator ei, edge_end;

  for (boost::tie(i, end) = vertices(g); i != end; ++i) {
    std::cout << id[*i] << " ";
    for (boost::tie(ei, edge_end) = out_edges(*i, g); ei != edge_end; ++ei)
      std::cout << " --" << name[*ei] << "--> " << id[target(*ei, g)] << "  ";
    std::cout << std::endl;
  }
  std::cout << std::endl;

  std::cout << "merging vertex 1 into vertex 0" << std::endl << std::endl;
  merge_vertex(0, 1, g, get_edge_name<graph_type>(g));
  
  for (boost::tie(i, end) = vertices(g); i != end; ++i) {
    std::cout << id[*i] << " ";
    for (boost::tie(ei, edge_end) = out_edges(*i, g); ei != edge_end; ++ei)
      std::cout << " --" << name[*ei] << "--> " << id[target(*ei, g)] << "  ";
    std::cout << std::endl;
  }
  std::cout << std::endl;
#endif  
  return 0;
}
